# Checks the integrity of u-blox binary files

# Written by: Paul Clark
# Last update: May 12th, 2025

# Reads a UBX file and checks the integrity of UBX, NMEA and RTCM data

# How it works:
#
# Each byte from the UBX input file fi is processed according to the ubx_nmea_state state machine
#
# For UBX messages:
# Sync Char 1: 0xB5
# Sync Char 2: 0x62
# Class byte
# ID byte
# Length: two bytes, little endian
# Payload: length bytes
# Checksum: two bytes
# E.g.:
# RXM_RAWX is class 0x02 ID 0x15
# RXM_SFRBF is class 0x02 ID 0x13
# TIM_TM2 is class 0x0d ID 0x03
# NAV_POSLLH is class 0x01 ID 0x02
# NAV_PVT is class 0x01 ID 0x07
# NAV-STATUS is class 0x01 ID 0x03
# Sync is lost when:
#   0x62 does not follow 0xB5
#   The checksum fails
#
# For NMEA messages:
# Starts with a '$'
# The next five characters indicate the message type (stored in nmea_char_1 to nmea_char_5)
# Message fields are comma-separated
# Followed by an '*'
# Then a two character checksum (the logical exclusive-OR of all characters between the $ and the * as ASCII hex)
# Ends with CR LF
# Sync is lost when:
#   The message length is excessive
#   The checksum fails
#   CR does not follow the checksum
#   LF does not follow CR
#
# For RTCM messages:
# Byte0 is 0xD3
# Byte1 contains 6 unused bits plus the 2 MS bits of the message length
# Byte2 contains the remainder of the message length
# Byte3 contains the first 8 bits of the message type
# Byte4 contains the last 4 bits of the message type and (optionally) the first 4 bits of the sub type
# Byte5 contains (optionally) the last 8 bits of the sub type
# Payload
# Checksum: three bytes CRC-24Q (calculated from Byte0 to the end of the payload, with seed 0)
# Sync is lost when:
#   The checksum fails
#
# This code will:
#   Rewind and re-sync if an error is found (sync is lost)
#   Create a repaired file if desired
#   Print any GNTXT messages if desired
#
# If sync is lost:
#   The ubx_nmea_state is set initially to sync_lost
#   sync_lost_at records the file byte at which sync was lost
#   resync_in_progress is set to True
#   The code attempts to resync - searching for the next valid message
#   Sync is re-established the next time a valid message is found. resync_in_progress is set to False
#
# Rewind:
#   If (e.g.) a UBX payload byte is dropped by the logging software,
#   the checksum bytes become misaligned and the checksum fails.
#   We do not know how many bytes were dropped...
#   If we attempt to re-sync immediately after the checksum failure
#   - without rewinding - the next valid message will also be discarded
#   as the first byte(s) of that message will already have been processed
#   when the checksum failure is detected.
#   To avoid this, rewind_to stores the position of the last known valid data.
#   E.g. rewind_to stores the position of the UBX length MSB byte - the byte
#   before the start of the payload. If a UBX payload byte is dropped and the
#   checksum fails, the code will rewind to that byte and attempt to re-sync
#   from there. The valid message following the erroneous one is then processed
#   correctly.
#   rewind_in_progress is set to True during a rewind and cleared when the next
#   valid message is processed. rewind_in_progress prevents the code from
#   rewinding more than once. The code will not rewind again until
#   rewind_in_progress is cleared.
#
# Repair:
#   This code can repair the file, copying only valid CRC-checked messages to the
#   repair file fo. When sync is lost and a rewind occurs, we need to rewind the
#   repair file to the start of the erroneous message, and overwrite it with
#   subsequent valid data.
#   rewind_repair_file_to contains the position of the end of the last valid
#   message written to the repair file. The repair file is rewound to here
#   during a rewind and re-sync.
#   The repair file is rewound and truncated before being closed, to discard any
#   possible partial message already copied to the file.


# SparkFun code, firmware, and software is released under the MIT License (http://opensource.org/licenses/MIT)
#
# The MIT License (MIT)
#
# Copyright (c) 2020 SparkFun Electronics
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.


import sys
import os

class UBX_Integrity_Checker():

    def __init__(self, ubxFile:str = None, repairFile:str = None, printGNTXT:bool = False, maxRewinds:int = 100):
        self.filename = ubxFile
        self.repairFilename = repairFile
        self.printGNTXT = printGNTXT
        self.max_rewinds = maxRewinds # Abort after this many rewinds

    def setFilename(self, ubxFile:str):
        self.filename = ubxFile

    def setRepairFilename(self, repairFile:str):
        self.repairFilename = repairFile

    def setPrintGNTXT(self, printGNTXT:bool):
        self.printGNTXT = printGNTXT

    def setMaxRewinds(self, maxRewinds:int):
        self.max_rewinds = maxRewinds

    # Add byte to checksums sum1 and sum2
    def csum(self, byte, sum1, sum2):
        sum1 = sum1 + byte
        sum2 = sum2 + sum1
        sum1 = sum1 & 0xFF
        sum2 = sum2 & 0xFF
        return sum1,sum2

    # Add byte to CRC-24Q (RTCM) checksum
    def crc24q(self, byte, sum):
        crc = sum # Seed is 0

        crc ^= byte << 16 # XOR-in incoming

        for i in range(8):
            crc <<= 1
            if (crc & 0x1000000):
                # CRC-24Q Polynomial:
                # gi = 1 for i = 0, 1, 3, 4, 5, 6, 7, 10, 11, 14, 17, 18, 23, 24
                # 0b 1 1000 0110 0100 1100 1111 1011
                crc ^= 0x1864CFB # CRC-24Q

        return crc & 0xFFFFFF


    def checkIntegrity(self):

        print('UBX Integrity Checker')
        print()

        if self.filename is None:
            raise Exception('Invalid file!')

        print('Processing',self.filename)
        print()
        filesize = os.path.getsize(self.filename) # Record the file size

        # Try to open file for reading
        try:
            fi = open(self.filename,"rb")
        except:
            raise Exception('Invalid file!')

        # Try to open repair file for writing
        fo = None
        if (self.repairFilename is not None):
            try:
                fo = open(self.repairFilename,"wb")
            except:
                fi.close()
                raise Exception('Could not open repair file!')

        processed = -1 # The number of bytes processed
        messages = {} # The collected message types
        keepGoing = True

        # Sync 'state machine'
        looking_for_B5_dollar_D3    = 0 # Looking for UBX 0xB5, NMEA '$' or RTCM 0xD3
        looking_for_62              = 1 # Looking for UBX 0x62 header byte
        looking_for_class           = 2 # Looking for UBX class byte
        looking_for_ID              = 3 # Looking for UBX ID byte
        looking_for_length_LSB      = 4 # Looking for UBX length bytes
        looking_for_length_MSB      = 5
        processing_UBX_payload      = 6 # Processing the UBX payload. Keep going until length bytes have been processed
        looking_for_checksum_A      = 7 # Looking for UBX checksum bytes
        looking_for_checksum_B      = 8
        sync_lost                   = 9 # Go into this state if sync is lost (bad checksum etc.)
        looking_for_asterix         = 10 # Looking for NMEA '*'
        looking_for_csum1           = 11 # Looking for NMEA checksum bytes
        looking_for_csum2           = 12
        looking_for_term1           = 13 # Looking for NMEA terminating bytes (CR and LF)
        looking_for_term2           = 14
        looking_for_RTCM_len1       = 15 # Looking for RTCM length byte (2 MS bits)
        looking_for_RTCM_len2       = 16 # Looking for RTCM length byte (8 LS bits)
        looking_for_RTCM_type1      = 17 # Looking for RTCM Type byte (8 MS bits, first byte of the payload)
        looking_for_RTCM_type2      = 18 # Looking for RTCM Type byte (4 LS bits, second byte of the payload)
        looking_for_RTCM_subtype    = 19 # Looking for RTCM Sub-Type byte (8 LS bits, third byte of the payload)
        processing_RTCM_payload     = 20 # Processing RTCM payload bytes
        looking_for_RTCM_csum1      = 21 # Looking for the first 8 bits of the CRC-24Q checksum
        looking_for_RTCM_csum2      = 22 # Looking for the second 8 bits of the CRC-24Q checksum
        looking_for_RTCM_csum3      = 23 # Looking for the third 8 bits of the CRC-24Q checksum

        ubx_nmea_state = sync_lost # Initialize the state machine

        # Storage for UBX messages
        ubx_length = 0
        ubx_class = 0
        ubx_ID = 0
        ubx_checksum_A = 0
        ubx_checksum_B = 0
        ubx_expected_checksum_A = 0
        ubx_expected_checksum_B = 0
        longest_UBX = 0 # The length of the longest UBX message
        longest_UBX_candidate = 0 # Candidate for the length of the longest valid UBX message

        # Storage for NMEA messages
        nmea_length = 0
        nmea_char_1 = 0 # e.g. G
        nmea_char_2 = 0 # e.g. P
        nmea_char_3 = 0 # e.g. G
        nmea_char_4 = 0 # e.g. G
        nmea_char_5 = 0 # e.g. A
        nmea_csum = 0
        nmea_csum1 = 0
        nmea_csum2 = 0
        nmea_expected_csum1 = 0
        nmea_expected_csum2 = 0
        longest_NMEA = 0 # The length of the longest valid NMEA message
        containsNMEA = False

        # Storage for RTCM messages
        rtcm_length = 0
        rtcm_type = 0
        rtcm_subtype = 0
        rtcm_expected_csum = 0
        rtcm_actual_csum = 0
        longest_rtcm = 0 # The length of the longest valid RTCM message
        longest_rtcm_candidate = 0 # Candidate for the length of the longest valid RTCM message
        containsRTCM = False

        max_nmea_len = 128 # Maximum length for an NMEA message: use this to detect if we have lost sync while receiving an NMEA message
        max_rtcm_len = 1023 + 6 # Maximum length for a RTCM message: use this to detect if we have lost sync while receiving a RTCM message
        sync_lost_at = -1 # Record where we lost sync
        rewind_to = -1 # Keep a note of where we should rewind to if sync is lost
        rewind_attempts = 0 # Keep a note of how many rewinds have been attempted
        rewind_in_progress = False # Flag to indicate if a rewind is in progress
        resyncs = 0 # Record the number of successful resyncs
        resync_in_progress = False # Flag to indicate if a resync is in progress
        message_start_byte = 0 # Record where the latest message started (for resync reporting)

        rewind_repair_file_to = 0 # Keep a note of where to rewind the repair file to if sync is lost
        repaired_file_bytes = 0 # Keep a note of how many bytes have been written to the repair file

        try:
            while keepGoing:

                # Read one byte from the file
                fileBytes = fi.read(1)
                if (len(fileBytes) == 0):
                    print('ERROR: Read zero bytes. End of file?! Or zero file size?!')
                    raise Exception('End of file?! Or zero file size?!')
                c = fileBytes[0]

                processed = processed + 1 # Keep a record of how many bytes have been read and processed

                # Write the byte to the repair file if desired
                if fo:
                    fo.write(fileBytes)
                    repaired_file_bytes = repaired_file_bytes + 1

                # Process each byte through the state machine
                if (ubx_nmea_state == looking_for_B5_dollar_D3) or (ubx_nmea_state == sync_lost):
                    if (c == 0xB5): # Have we found Sync Char 1 (0xB5) if we were expecting one?
                        if (ubx_nmea_state == sync_lost):
                            print("UBX Sync Char 1 (0xB5) found at byte "+str(processed))
                            print()
                        ubx_nmea_state = looking_for_62 # Now look for Sync Char 2 (0x62)
                        message_start_byte = processed # Record the message start byte for resync reporting
                    elif (c == 0x24): # Have we found an NMEA '$' if we were expecting one?
                        if (ubx_nmea_state == sync_lost):
                            print("NMEA $ found at byte "+str(processed))
                            print()
                        ubx_nmea_state = looking_for_asterix # Now keep going until we receive an asterix
                        containsNMEA = True
                        nmea_length = 0 # Reset nmea_length then use it to check for excessive message length
                        nmea_csum = 0 # Reset the nmea_csum. Update it as each character arrives
                        nmea_char_1 = 0x30 # Reset the first five NMEA chars to something invalid
                        nmea_char_2 = 0x30
                        nmea_char_3 = 0x30
                        nmea_char_4 = 0x30
                        nmea_char_5 = 0x30
                        message_start_byte = processed # Record the message start byte for resync reporting
                        nmea_string = None
                        if self.printGNTXT == True:
                            nmea_string = '$'
                    elif (c == 0xD3): # Have we found 0xD3 if we were expecting one?
                        if (ubx_nmea_state == sync_lost):
                            print("RTCM 0xD3 found at byte "+str(processed))
                            print()
                        ubx_nmea_state = looking_for_RTCM_len1 # Now keep going until we receive the checksum
                        containsRTCM = True
                        rtcm_expected_csum = 0 # Reset the RTCM csum with a seed of 0. Update it as each character arrives
                        rtcm_expected_csum = self.crc24q(c, rtcm_expected_csum) # Update expected checksum
                        message_start_byte = processed # Record the message start byte for resync reporting
                    else:
                        #print("Was expecting Sync Char 0xB5, NMEA $ or RTCM 0xD3 but did not receive one!")
                        sync_lost_at = processed
                        ubx_nmea_state = sync_lost
                
                # UBX messages
                elif (ubx_nmea_state == looking_for_62):
                    if (c == 0x62): # Have we found Sync Char 2 (0x62) when we were expecting one?
                        ubx_expected_checksum_A = 0 # Reset the expected checksum
                        ubx_expected_checksum_B = 0
                        ubx_nmea_state = looking_for_class # Now look for Class byte
                    else:
                        print("Panic!! Was expecting Sync Char 2 (0x62) but did not receive one!")
                        print("Sync lost at byte "+str(processed)+". Attemting to re-sync")
                        print()
                        sync_lost_at = processed
                        resync_in_progress = True
                        ubx_nmea_state = sync_lost
                elif (ubx_nmea_state == looking_for_class):
                    ubx_class = c
                    ubx_expected_checksum_A, ubx_expected_checksum_B = self.csum(c, ubx_expected_checksum_A, ubx_expected_checksum_B)
                    ubx_nmea_state = looking_for_ID # Now look for ID byte
                elif (ubx_nmea_state == looking_for_ID):
                    ubx_ID = c
                    ubx_expected_checksum_A, ubx_expected_checksum_B = self.csum(c, ubx_expected_checksum_A, ubx_expected_checksum_B)
                    message_type = '0x%02X 0x%02X'%(ubx_class,ubx_ID) # Record the message type
                    ubx_nmea_state = looking_for_length_LSB # Now look for length LSB
                elif (ubx_nmea_state == looking_for_length_LSB):
                    ubx_length = c # Store the length LSB
                    ubx_expected_checksum_A, ubx_expected_checksum_B = self.csum(c, ubx_expected_checksum_A, ubx_expected_checksum_B)
                    ubx_nmea_state = looking_for_length_MSB # Now look for length MSB
                elif (ubx_nmea_state == looking_for_length_MSB):
                    ubx_length = ubx_length + (c * 256) # Add the length MSB
                    ubx_expected_checksum_A, ubx_expected_checksum_B = self.csum(c, ubx_expected_checksum_A, ubx_expected_checksum_B)
                    longest_UBX_candidate = ubx_length + 8 # Update the longest UBX message length candidate. Include the header, class, ID, length and checksum bytes
                    rewind_to = processed # If we lose sync due to dropped bytes then rewind to here
                    if ubx_length > 0:
                        ubx_nmea_state = processing_UBX_payload # Now look for payload bytes (length: ubx_length)
                    else:
                        ubx_nmea_state = looking_for_checksum_A # Zero length message. Go straight to checksum A
                elif (ubx_nmea_state == processing_UBX_payload):
                    ubx_length = ubx_length - 1 # Decrement length by one
                    ubx_expected_checksum_A, ubx_expected_checksum_B = self.csum(c, ubx_expected_checksum_A, ubx_expected_checksum_B)
                    if (ubx_length == 0):
                        ubx_expected_checksum_A = ubx_expected_checksum_A & 0xff # Limit checksums to 8-bits
                        ubx_expected_checksum_B = ubx_expected_checksum_B & 0xff
                        ubx_nmea_state = looking_for_checksum_A # If we have received length payload bytes, look for checksum bytes
                elif (ubx_nmea_state == looking_for_checksum_A):
                    ubx_checksum_A = c
                    ubx_nmea_state = looking_for_checksum_B
                elif (ubx_nmea_state == looking_for_checksum_B):
                    ubx_checksum_B = c
                    ubx_nmea_state = looking_for_B5_dollar_D3 # All bytes received so go back to looking for a new Sync Char 1 unless there is a checksum error
                    if ((ubx_expected_checksum_A != ubx_checksum_A) or (ubx_expected_checksum_B != ubx_checksum_B)):
                        print("Panic!! UBX checksum error!")
                        print("Sync lost at byte "+str(processed)+". Attemting to re-sync.")
                        print()
                        sync_lost_at = processed
                        resync_in_progress = True
                        ubx_nmea_state = sync_lost
                    else:
                        # Valid UBX message was received. Check if we have seen this message type before
                        if message_type in messages:
                            messages[message_type] += 1 # if we have, increment its count
                        else:
                            messages[message_type] = 1 # if we have not, set its count to 1
                        if (longest_UBX_candidate > longest_UBX): # Update the longest UBX message length
                            longest_UBX = longest_UBX_candidate
                        rewind_in_progress = False # Clear rewind_in_progress
                        rewind_to = -1
                        if (resync_in_progress == True): # Check if we are resyncing
                            resync_in_progress = False # Clear the flag now that a valid message has been received
                            resyncs += 1 # Increment the number of successful resyncs
                            print("Sync successfully re-established at byte "+str(processed)+". The UBX message started at byte "+str(message_start_byte))
                            print()
                            if (fo):
                                fo.seek(rewind_repair_file_to) # Rewind the repaired file
                                repaired_file_bytes = rewind_repair_file_to
                                fi.seek(message_start_byte) # Copy the valid message into the repair file
                                repaired_bytes_to_write = 1 + processed - message_start_byte
                                fileBytes = fi.read(repaired_bytes_to_write)
                                fo.write(fileBytes)
                                repaired_file_bytes = repaired_file_bytes + repaired_bytes_to_write
                                rewind_repair_file_to = repaired_file_bytes
                        else:
                            if (fo):
                                rewind_repair_file_to = repaired_file_bytes # Rewind repair file to here if sync is lost

                # NMEA messages
                elif (ubx_nmea_state == looking_for_asterix):
                    nmea_length = nmea_length + 1 # Increase the message length count
                    if nmea_string is not None:
                        nmea_string += chr(c)
                    if (nmea_length > max_nmea_len): # If the length is greater than max_nmea_len, something bad must have happened (sync_lost)
                        print("Panic!! Excessive NMEA message length!")
                        print("Sync lost at byte "+str(processed)+". Attemting to re-sync")
                        print()
                        sync_lost_at = processed
                        resync_in_progress = True
                        ubx_nmea_state = sync_lost
                        nmea_string = None
                        continue
                    # If this is one of the first five characters, store it
                    if (nmea_length <= 5):
                        if (nmea_length == 1):
                            nmea_char_1 = c
                            rewind_to = processed # If we lose sync due to dropped bytes then rewind to here
                        elif (nmea_length == 2):
                            nmea_char_2 = c
                        elif (nmea_length == 3):
                            nmea_char_3 = c
                        elif (nmea_length == 4):
                            nmea_char_4 = c
                        else: # ubx_length == 5
                            nmea_char_5 = c
                            if (nmea_char_5 == ','): # Check for a 4-character message type (e.g. PUBX)
                                message_type = chr(nmea_char_1) + chr(nmea_char_2) + chr(nmea_char_3) + chr(nmea_char_4)
                            else:
                                message_type = chr(nmea_char_1) + chr(nmea_char_2) + chr(nmea_char_3) + chr(nmea_char_4) + chr(nmea_char_5) # Record the message type
                            if (message_type != "GNTXT"): # Reset nmea_string if this is not GNTXT
                                nmea_string = None
                    # Now check if this is an '*'
                    if (c == 0x2A):
                        # Asterix received
                        # Don't exOR it into the checksum
                        # Instead calculate what the expected checksum should be (nmea_csum in ASCII hex)
                        nmea_expected_csum1 = ((nmea_csum & 0xf0) >> 4) + 0x30 # Convert MS nibble to ASCII hex
                        if (nmea_expected_csum1 >= 0x3A): # : follows 9 so add 7 to convert to A-F
                            nmea_expected_csum1 += 7
                        nmea_expected_csum2 = (nmea_csum & 0x0f) + 0x30 # Convert LS nibble to ASCII hex
                        if (nmea_expected_csum2 >= 0x3A): # : follows 9 so add 7 to convert to A-F
                            nmea_expected_csum2 += 7
                        # Next, look for the first csum character
                        ubx_nmea_state = looking_for_csum1
                        continue # Don't include the * in the checksum
                    # Now update the checksum
                    # The checksum is the exclusive-OR of all characters between the $ and the *
                    nmea_csum = nmea_csum ^ c
                elif (ubx_nmea_state == looking_for_csum1):
                    # Store the first NMEA checksum character
                    nmea_csum1 = c
                    if nmea_string is not None:
                        nmea_string += chr(c)
                    ubx_nmea_state = looking_for_csum2
                elif (ubx_nmea_state == looking_for_csum2):
                    # Store the second NMEA checksum character
                    nmea_csum2 = c
                    # Now check if the checksum is correct
                    if ((nmea_csum1 != nmea_expected_csum1) or (nmea_csum2 != nmea_expected_csum2)):
                        # The checksum does not match so sync_lost
                        print("Panic!! NMEA checksum error!")
                        print("Sync lost at byte "+str(processed)+". Attemting to re-sync")
                        print()
                        sync_lost_at = processed
                        resync_in_progress = True
                        ubx_nmea_state = sync_lost
                        nmea_string = None
                    else:
                        # Checksum was valid so wait for the terminators
                        if nmea_string is not None:
                            nmea_string += chr(c)
                        ubx_nmea_state = looking_for_term1
                elif (ubx_nmea_state == looking_for_term1):
                    # Check if this is CR
                    if (c != 0x0D):
                        print("Panic!! NMEA CR not found!")
                        print("Sync lost at byte "+str(processed)+". Attemting to re-sync")
                        print()
                        sync_lost_at = processed
                        resync_in_progress = True
                        ubx_nmea_state = sync_lost
                        nmea_string = None
                    else:
                        if nmea_string is not None:
                            nmea_string += chr(c)
                        ubx_nmea_state = looking_for_term2
                elif (ubx_nmea_state == looking_for_term2):
                    # Check if this is LF
                    if (c != 0x0A):
                        print("Panic!! NMEA LF not found!")
                        print("Sync lost at byte "+str(processed)+". Attemting to re-sync")
                        print()
                        sync_lost_at = processed
                        resync_in_progress = True
                        ubx_nmea_state = sync_lost
                        nmea_string = None
                    else:
                        # Valid NMEA message was received. Check if we have seen this message type before
                        if message_type in messages:
                            messages[message_type] += 1 # if we have, increment its count
                        else:
                            messages[message_type] = 1 # if we have not, set its count to 1
                        if (nmea_length > longest_NMEA): # Update the longest NMEA message length
                            longest_NMEA = nmea_length
                        # Print GNTXT
                        if nmea_string is not None and self.printGNTXT == True:
                            nmea_string += chr(c)
                            print(nmea_string)
                            nmea_string = None
                        # LF was received so go back to looking for B5 or a $
                        ubx_nmea_state = looking_for_B5_dollar_D3
                        rewind_in_progress = False # Clear rewind_in_progress
                        rewind_to = -1
                        if (resync_in_progress == True): # Check if we are resyncing
                            resync_in_progress = False # Clear the flag now that a valid message has been received
                            resyncs += 1 # Increment the number of successful resyncs
                            print("Sync successfully re-established at byte "+str(processed)+". The NMEA message started at byte "+str(message_start_byte))
                            print()
                            if (fo):
                                fo.seek(rewind_repair_file_to) # Rewind the repaired file
                                repaired_file_bytes = rewind_repair_file_to
                                fi.seek(message_start_byte) # Copy the valid message into the repair file
                                repaired_bytes_to_write = 1 + processed - message_start_byte
                                fileBytes = fi.read(repaired_bytes_to_write)
                                fo.write(fileBytes)
                                repaired_file_bytes = repaired_file_bytes + repaired_bytes_to_write
                                rewind_repair_file_to = repaired_file_bytes
                        else:
                            if (fo):
                                rewind_repair_file_to = repaired_file_bytes # Rewind repair file to here if sync is lost
                
                # RTCM messages
                elif (ubx_nmea_state == looking_for_RTCM_len1):
                    rtcm_length = (c & 0x03) << 8 # Extract length
                    rtcm_expected_csum = self.crc24q(c, rtcm_expected_csum) # Update expected checksum
                    rewind_to = processed # If we lose sync due to dropped bytes then rewind to here
                    ubx_nmea_state = looking_for_RTCM_len2
                elif (ubx_nmea_state == looking_for_RTCM_len2):
                    rtcm_length |= c # Extract length
                    longest_rtcm_candidate = rtcm_length + 6 # Update the longest RTCM message length candidate. Include the header, length and checksum bytes
                    rtcm_expected_csum = self.crc24q(c, rtcm_expected_csum) # Update expected checksum
                    if (rtcm_length > 0):
                        ubx_nmea_state = looking_for_RTCM_type1
                    else:
                        ubx_nmea_state = looking_for_RTCM_csum1
                elif (ubx_nmea_state == looking_for_RTCM_type1):
                    rtcm_type = c << 4 # Extract type
                    rtcm_expected_csum = self.crc24q(c, rtcm_expected_csum) # Update expected checksum
                    rtcm_length = rtcm_length - 1 # Decrement length by one
                    if (rtcm_length > 0):
                        ubx_nmea_state = looking_for_RTCM_type2
                    else:
                        ubx_nmea_state = looking_for_RTCM_csum1
                elif (ubx_nmea_state == looking_for_RTCM_type2):
                    rtcm_type |= c >> 4 # Extract type
                    message_type = '%04i'%rtcm_type # Record the message type
                    rtcm_subtype = (c & 0x0F) << 8 # Extract sub-type
                    rtcm_expected_csum = self.crc24q(c, rtcm_expected_csum) # Update expected checksum
                    rtcm_length = rtcm_length - 1 # Decrement length by one
                    if (rtcm_length > 0):
                        ubx_nmea_state = looking_for_RTCM_subtype
                    else:
                        ubx_nmea_state = looking_for_RTCM_csum1
                elif (ubx_nmea_state == looking_for_RTCM_subtype):
                    rtcm_subtype |= c # Extract sub-type
                    if (rtcm_type == 4072): # Record the sub-type but only for 4072 messages
                        message_type = message_type + '_%i'%rtcm_subtype
                    rtcm_expected_csum = self.crc24q(c, rtcm_expected_csum) # Update expected checksum
                    rtcm_length = rtcm_length - 1 # Decrement length by one
                    if (rtcm_length > 0):
                        ubx_nmea_state = processing_RTCM_payload
                    else:
                        ubx_nmea_state = looking_for_RTCM_csum1
                elif (ubx_nmea_state == processing_RTCM_payload):
                    rtcm_expected_csum = self.crc24q(c, rtcm_expected_csum) # Update expected checksum
                    rtcm_length = rtcm_length - 1 # Decrement length by one
                    if (rtcm_length == 0):
                        ubx_nmea_state = looking_for_RTCM_csum1
                elif (ubx_nmea_state == looking_for_RTCM_csum1):
                    rtcm_actual_csum = c << 8
                    ubx_nmea_state = looking_for_RTCM_csum2
                elif (ubx_nmea_state == looking_for_RTCM_csum2):
                    rtcm_actual_csum |= c
                    rtcm_actual_csum <<= 8
                    ubx_nmea_state = looking_for_RTCM_csum3
                elif (ubx_nmea_state == looking_for_RTCM_csum3):
                    rtcm_actual_csum |= c
                    ubx_nmea_state = looking_for_B5_dollar_D3 # All bytes received so go back to looking for a new Sync Char 1 unless there is a checksum error
                    if (rtcm_expected_csum != rtcm_actual_csum):
                        print("Panic!! RTCM checksum error!")
                        print("Sync lost at byte "+str(processed)+". Attemting to re-sync.")
                        print()
                        sync_lost_at = processed
                        resync_in_progress = True
                        ubx_nmea_state = sync_lost
                    else:
                        # Valid RTCM message was received. Check if we have seen this message type before
                        if (longest_rtcm_candidate >= 8): # Message must contain at least 2 (+6) bytes to include a valid mesage type
                            if message_type in messages:
                                messages[message_type] += 1 # if we have, increment its count
                            else:
                                messages[message_type] = 1 # if we have not, set its count to 1
                            if (longest_rtcm_candidate > longest_rtcm): # Update the longest RTCM message length
                                longest_rtcm = longest_rtcm_candidate
                        rewind_in_progress = False # Clear rewind_in_progress
                        rewind_to = -1
                        if (resync_in_progress == True): # Check if we are resyncing
                            resync_in_progress = False # Clear the flag now that a valid message has been received
                            resyncs += 1 # Increment the number of successful resyncs
                            print("Sync successfully re-established at byte "+str(processed)+". The RTCM message started at byte "+str(message_start_byte))
                            print()
                            if (fo):
                                fo.seek(rewind_repair_file_to) # Rewind the repaired file
                                repaired_file_bytes = rewind_repair_file_to
                                fi.seek(message_start_byte) # Copy the valid message into the repair file
                                repaired_bytes_to_write = 1 + processed - message_start_byte
                                fileBytes = fi.read(repaired_bytes_to_write)
                                fo.write(fileBytes)
                                repaired_file_bytes = repaired_file_bytes + repaired_bytes_to_write
                                rewind_repair_file_to = repaired_file_bytes
                        else:
                            if (fo):
                                rewind_repair_file_to = repaired_file_bytes # Rewind repair file to here if sync is lost


                # Check if the end of the file has been reached
                if (processed >= filesize - 1):
                    keepGoing = False

                # Check if we should attempt to rewind
                # Don't rewind if we have not yet seen a valid message
                # Don't rewind if a rewind is already in progress
                # Don't rewind if at the end of the file
                if (ubx_nmea_state == sync_lost) and (len(messages) > 0) and (rewind_in_progress == False) and (rewind_to >= 0) and (keepGoing):
                    rewind_attempts += 1 # Increment the number of rewind attempts
                    if (rewind_attempts > self.max_rewinds): # Only rewind up to max_rewind times
                        print("Panic! Maximum rewind attempts reached! Aborting...")
                        keepGoing = False
                    else:
                        print("Sync has been lost. Currently processing byte "+str(processed)+". Rewinding to byte "+str(rewind_to))
                        print()
                        fi.seek(rewind_to) # Rewind the file
                        processed = rewind_to - 1 # Rewind processed too! (-1 is needed as processed is incremented at the start of the loop)
                        rewind_in_progress = True # Flag that a rewind is in progress
                    

        finally:
            fi.close() # Close the file

            if (fo):
                fo.seek(rewind_repair_file_to) # Discard any partial message at the very end of the repair file
                fo.truncate()
                fo.close()
                
            # Print the file statistics
            print()
            processed += 1
            print('Processed',processed,'bytes')
            print('File size was',filesize)
            if (processed != filesize):
                print('FILE SIZE MISMATCH!!')
            print('Longest valid UBX message was %i bytes'%longest_UBX)
            if (containsNMEA == True):
                print('Longest valid NMEA message was %i characters'%longest_NMEA)
            if (containsRTCM == True):
                print('Longest valid RTCM message was %i bytes'%longest_rtcm)
            if len(messages) > 0:
                print('Message types and totals were:')
                for key in messages.keys():
                    spaces = ' ' * (9 - len(key))
                    print('Message type:',key,spaces,'Total:',messages[key])
            if (resyncs > 0):
                print('Number of successful resyncs:',resyncs)
            print()
            if (fo):
                print('Repaired data written to:', self.repairFilename)
            print()

if __name__ == '__main__':

    import argparse

    parser = argparse.ArgumentParser(description='SparkFun UBX Integrity Checker')
    parser.add_argument('ubxFile', metavar='ubxFile', type=str, help='The path to the UBX file')
    parser.add_argument('-r', '--repairFile', required=False, type=str, default=None, help='The path to the repair file')
    parser.add_argument('--GNTXT', default=False, action='store_true', help='Display any GNTXT messages found')
    parser.add_argument('-rw', '--rewinds', required=False, type=int, default=100, help='The maximum number of file rewinds')
    args = parser.parse_args()

    checker = UBX_Integrity_Checker(args.ubxFile, args.repairFile, args.GNTXT, args.rewinds)

    checker.checkIntegrity()
