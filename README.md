# Voice encrypting app on PC under Windows

Open source variant of JackPair on PC under Windows 98 to 10

This is a Windows software of a digital scrambler for speech encryption in analogue HF /VHF radio and 
GSM/WCDMA audio compressed links. It is fully compatible with the hardcore device on STM32F446RE 
Nucleo Board. Project is an attempt to implement an open source JackPair. It has nothing to do with the 
original JackPair project. We can not even compare the functionality since we do not have the original 
JackPair or other similar devices. This project is for education purposes only: some licensed codes are inside 
(see cover in each file). In fact this device is similar to that developed in the laboratory in Marfino (Moscow, 
USSR) on the orders of Stalin in early 50's (see Solzhenitsyn's novel "The First Circle").

Our device allows to point-to-point encrypt speech using an analog audio interface. It is in the form of a 
hardware headset connected to a VHF radio or mobile phone using Bluetooth or audio connectors. Both full 
duplex and simplex (PTT mode) is supported. 12-bit daily code is set using switches to protect from 
unauthorized access (in old military style). The principle of the device is to convert speech into a low bitrate 
data stream by a speech codec, provide encryption and then modulate into a pseudo-speech signal by a 
special modem.

Functional description of modules:

Audio codec: the MELPE-1200 algorithm with NPP7 noise suppressor is used. The original codec works with 
67.5 ms frames containing 540 8KHz PCM samples, compressing them to 81 bits of data. To achieve the 
required bitrate 800 bps, we eliminate sync bit and 8 bits of Fourier magnitudes, which did not significantly 
affect the quality of speech. In addition, we reduced PCM sampling rate to 6KHz, which is also acceptable for 
most speakers (possibly, except very high female and child voices). Thus, the voice codec used in the project 
provides a bitrate of 800 bps, works with 90 ms frames containing 540 6KHz PCM samples, compressing them 
to 72 bits of data. The quality of speech practically not differs from the original MELPE-1200 algorithm.
Encrypting is in stream mode with no error spreading. Keccak-800 Sponge permutation is used as a XOF like 
Shake-128 that absorbs the session key, one-way counter and squeezes gamma XOR with speech data 
frame. Synchronization of counters is provided in speech pauses. 128-bits Encryption and decryption keys are 
output at the beginning of the session by the authenticated protocol SPEKE based on Diffie-Hellmann with 
base point derived from authenticator. This protocol provides zero knowledge and it is resistant to offline 
attacks, which allows you to use a short pre-shared secret as a 12-bit daily code. Key exchange is executed 
with an elliptic curve X25519, Hash2Point function uses Elligator2 algorithm. TRNG is charged with one LSB 
of the white noise on power voltage resistive divider. Accumulated entropy is estimated by 4-bits block 
frequency test, gathering is performed to a sufficient statistic. All cryptographic primitives are implemented on 
Assembler and strictly adapted to the Cortex M4 platform with constant time and minimization of possible leaks 
through current consumption and EM emitted side channels
Modems are specially designed BPSK for VHF radio and Pulse for GSM FR / AMR compressed channels. 
Both provide a bitrate of 800 bps and work with 90 ms frames of 720 8 kHz samples, carrying a payload of 72 
bits, which corresponds to the frames of the audio codec. Modems are self-synchronizing and provide reliable 
synchronization during 300-500 ms even under the noise level. Digital processing is performed exclusively in a 
time domain and requires very few resources, which allowed detecting from all possible one sample lags, 
significantly reducing the effect of spontaneous phase jumps available in compressed audio channels. To 
accurately adjust the frequency and phase of sampling the resumpler from 48KHz Line recorded sample to 
8KHz is used. The trick of twice fading each even frame amplitude is used for prevent VAD mute the non-
speech signal.
BPSK modem details: Carrier is 1333Hz (exactly 6 8KHz samples per period). One bit codes exactly by 1.5 
periods (9 samples). Continuous phase uses (phase jumps smoothed for optimizing bandwidth). Each 90 mS 
frame contains 720 8KHz PCM samples carries 80 raw bits. Input data stream (72 bits) spited to 8 subframes 9 
bits each. Each subframe appends by one parity bit. Data interleaved before modulation: the first modulated 
bits 0 of all subframes, then bits 1, and last are parity bits of all subframes (in reverse order: from last to first). 
Demodulator processes 720 PCM samples over 9 samples overlapped with previous frame. Non-coherent 
detecting is more robust in non-AWGN channels (codecs output). Demodulator tries to detect bit in each 
sample lag so we have 9 variants of each bit and only one is good. For discovering of good sample lag and 
good bit lag (frame boundary) we continuously check parity of all previously received subframes assuming the 
received bit is a last bit of frame. The number of parity errors for each sample position add to array for last 10 
processed frames. After each frame was processed demodulator search in this array the best sample position 
is a frame boundary. If it is stable the sync is considered lock and demodulator will output aligned data. The 
simplest soft error correction is performed: if in a subframe the parity bit does not match, the bit with the 
minimum metric is inverted. The resulting output is hard bits and soft metrics used for HARQ during key 
exchange.

Pulse modem details: Our Pulse modem is variant of Katugampala & Kondoz modem developed in Surray 
University. We also took into account the improvements offered by Andreas Tyrberg from "Sectra 
Communication" in his Master thesis. This type of modem also used in European Automatic Emergency Call 
System (eCall) developed by Qualcomm. Special symbols are used for data transferring. The length of symbol 
is 24 8KHz PCM samples and each symbol contain one pulse on one of four possible positions: occupies 
samples 3,4,5 or 9,10,11 or 15,16,17 or 21,22,23. Each pulse is actually waveform (same as the shaped 
pulses of Kondoz’s modem) and can be positive or negative. So each symbol carries 3 bits: two codes pulse 
position and one - pulse polarity. Each 90 mS frame contains 720 8KHz PCM samples carries 30 symbols, so 
total number of bits are 90. Input data stream (72 bits) spited to 18 subframes 4 bits each. Each subframe 
appends by one parity bit. Data interleaved before modulation: the first modulated bits 0 of all subframes, then 
bits 1, and last are parity bits of all subframes (in reverse order: from last to first). For modulation data stream 
divides to 30 symbols 3 bits each and each symbol modulates to 24 8KHz PCM samples. Demodulator 
process 720 PCM samples over 24 samples overlapped with previous frame. Demodulation algorithm the 
same: demodulator tries to demodulate in each sample lag and then search the best lag as a frame boundary. 
Non-coherent detecting is used. Demodulator provides fast and/or accurate detecting, the last one comes 
out only on correct symbol lags. Some kinds of sync are used during processing: polarity check, selecting of 
correct pulse edge for effective non-coherent detecting, tune of sampling rate for more efficient pulse 
discovering etc. The data output of pulse demodulator is the same of data output of BPSK demodulator 
described early.

User interface: Application  converts a speech signal into data using modified MELPE codec on 800 bps, 
encrypts and then modulates data with BPSK or Pulse modem to line. Trick with periodic amplitude changing 
can be applied for  Demodulator provides fast synchronization from anywhere in the stream, adjust frequency 
and phase  and hold synchronization at deep fading on multipath interference and tropospheric link.

Both digital and transparent analog communication modes are supported. Simplex, half duplex and full duplex 
is supported. Hardware control (input and output of PTT and switching to transparent mode) is supported 
using COM-port. Both encrypted  and unencrypted (for HAM) digital modes and transparent (direct)  analogue 
mode are supported. On encryption the 128-bit level P2P encryption is provided with perfect forward secrecy 
(key exchange is performed at the beginning of the each new session). SHAKE128 based on Keccak-800 
permutation is used for symmetric encryption and hashing. The Diffie-Hellman protocol is executed on the 
elliptic curve X25519 and authenticated using zero-knowledge SPECE protocol with Elligator2  for hashing to 
the curve.

Audio processing is cross-platform code made on C and run as a one thread. The graphical interface is made 
using VCL on Borland C++ Builder 6. Executable is completely portable and does not write to the file system 
except in its working folder. User configuration is stored in the ini file.  PC should have two audio recording 
devices and two audio playback devices (sound cards). For example they can be sound card on motherboard 
and a USB audio card or Bluetooth headset. Devices for microphone, headphones, line input and output 
setup by user individually on the tabs of the program window.

You can also connect the USB-COM bridge and select the appropriate COM port for hardware control of PTT: 
with external button (input) or to radio (output). CTS input external PTT buttons,  DSR input switch 
totransparent mode, RTS outputs PTT state to radio and DTR outputs switch  to duplex mode. All input and 
output signals have active level is ground (0). Also the log of modem statistic is outputted over COM port on 
115200 bps  allowing estimate the quality of the communication channel. A BER is evaluated at the key 
exchange stage: one of the parties starts a continuous key transfer. The other side accepts the key, checks 
its checksum and then compares next iterations  with the accepted key for BER estimation. 

Users interface consists of the main tab (PTT) and tabs for settings: HEADSET for selecting audio devices for 
microphone and headphones, LINE for selecting audio devices for input and output of signalsto the line 
(connected to the radio), COM for selecting COM-port for hardware control and KEY - for setting the shared 
secret. If shared secret is empty string the encryption is not used and key exchange is not performed before 
the new session. The PTT tab has a PTT button for activate transmitting (this button can be also hold/release 
by right mouse button click), check boxes for  run of audio processing, apply the duplex mode (in this mode 
the receiving will continue during PTT button press) and apply the transparent mode (in this mode voice from 
microphone will be directly transmitted to Line provide analogue communication). The case of thread 
successful open the COM-port will be indicated with "HW cntr" label.  4 indicators show program status. The 
"Ready" indicator (blue) lights up after the key exchange or immediately after the start if encryption is not 
used. The "Key" indicator (yellow) lights up after the key exchange (at the beginning of a new session each 
of the parties must hold the PTT button on 2-3 seconds so that the other side receives the key). The "Lock" 
indicator (red) lights up if a carrier signal is detected in the line. In this case the speaker automatically switches 
to a digital voice. The "VAD" indicator is monitored by a microphone and lights up when the operator speaks 
(during speech pauses the device transmits the service information necessary for the synchronization of 
counters on both sides).

Note: Unfortunately many modern smartphones have in-build noise suppressor significantly reject modem 
base-band signal. So this feature must be disabled on engineering menu or with special audio configuration 
tool. 

Statistics output is available via the COM port. You can use Putty terminal tool (on Serial mode with 115200 
baudrate). On key exchange the extended statistics are outputted:

D - sequentional number of last received data packet;
P - packets errors rate (percent and absolute);
S - internal parity errors rate (percent and absolute);
B - bits errors rate comparing received control data with expected (percent and absolute);
L - PCM stream lag in samples (0-719), will be stable while carrier locked;
C - carrier locked (0/1), channel polarity (0/1), sampling rate tuning value (+-8) and average sampling 
frequency difference between parties.
In talk mode the BER is not outputs.
Adds TX flag (X - clear mode, C-silency: control data will be sent, V - voice sent) and RX flag (X - no lock of 
carrier, clear mode, C-silency: control data were receved, V-voice received).

For check channel quality run one device for discontinuos transmitting a key data and other device for 
receiving. Key was received and next frames will be compared with key data for estimate BER.

http://torfone.org/jackpair MailTo: torfone@ukr.net Van Gegel, 2018
