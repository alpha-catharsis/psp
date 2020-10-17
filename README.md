# Packet Synchronization Protocol (PSP)

## Important Notice

At the current stage this project has to be considered just a proof-of-concept. It works but it is very basic and requires some manual tuning
in order to provide good performance. If you use this software please provide your feedback to
[alpha.catharsis@gmail.com](mailto:alpha.catharsis@gmail.com).

## Introduction

PSP is an **_experimental_** time synchronization protocol for IP networks which has been devised to cover a small niche of use cases.
It can be employed when time synchronization via GNSS (e.g. GPS, Galileo, ...) is not available and when time synchronization through
standard IP protocols (i.e. NTP and PTP) is not viable. In particular PSP may be useful in the following situations:

* Time synchronization process must strictly converge within a required time

* Latency between machines involved in the time synchronization process is significantly asymmetric

* Only unidirectional IP traffic between the synchronization master and slave machines is allowed or preferred

* Very limited bandwidth is available or can be used for time synchronization purposes

With PSP is it possible synchronize the clock of a computer, indentified as _slave_, to the clock of another computer, identified as _master_,
through the uniderctional sending of IP packets from the master to the slave. To perform such synchronization, the slave needs to know the latency
statistics of the IP communication channel. For this reason PSP is based on a three-step approach: _pre-calibration_, _calibration_ and _synchronization_
steps.

The pre-calibration step allows the slave to estimate the frequency offset between the master and the slave clocks. The knowledge of the frequency
offset is necessary to correctly estimate the IP channel latency statistics during the calibration step.

The calibration step allows the slave to estimate the IP channel latency statistics. In this step the master and slave must be synchronized to a
common time through other means (e.g. GNSS or NTP/PTP). During calibration step the master periodically sends timestamp packets to the slave, which
measures the channel latency statistics.

Once calibration has been performed, it is possible to disconnect the master and slave from the common time source and proceed with the
synchronization through PSP protocol. During this step, the synchronization master periodically sends timestamp packets to the slave that,
based on recorded channel latency statistics, corrects its clock.

It is worth noting that the master always performs the same operation (i.e. timestamp packet transmission) during pre-calibration, calibration and
synchronization steps.

The accuracy of PSP protocol strongly depends on the calibration error and on the stability of channel latency statistics over time. Calibration
error depends on the accuracy of the master/slave synchronization during calibration and on the time spent during calibration for the
estimation of channel latency statistics. The stability of the channel latency statistics may vary significantly based on the network topology and
media. Best stability is achieved on low-loaded LANs and Wifi networks, while on internet the stability is much less predictable.

From a limited set of experiments it results that an accuracy of tens of microseconds is achievable on wired LANs, and that an accuracy of
tens of milliseconds is achievable through internet.

## Installation

PSP currently can only be installed on Linux and does not require additional dependencies. In order to install PSP the following commands shall be
executed from the shell:
~~~~
tar -xzvf psp-2.0.0.tar.gz
cd psp-2.0.0
./autogen.sh
./configure
make
make install
~~~~

Notes:

* The command `make install` requires root priviledges. If the user is not root but can use sudo, the following command can be used instead `sudo make
install`

* By default the executables and man pages are installed in `/usr/local/` instead of just `/usr/`. The pspm and pspm installed in `/usr/local/bin`
might not be in the user PATH environment variable. To install the software in `/usr/` the following command shall be used for configuring the
package `./configure --prefix=/usr`.

## Protocol description

During pre-calibration:

* Master periodically sends timestamp packets to the slave

* Slave estimates the relative frequency offset between its clock and master's clock

At the end of the pre-calibration step, the slaves save in the file 'precalibr\_results.txt' the estimated frequency offset.

During calibration:

* Master and slave are synchronized with good accuracy to a common time reference through other means

* Master periodically sends timestamp packets to the slave

* Slave measures the channel latency subtracting the received master timestamp from its local timestamp

At the end of the pre-calibration step, the slaves save in the file 'precalibr\_results.txt' the estimated IP channel median latency.

During synchronization:

* The master and the slave are not synchronized to the a common time reference through other means

* The master periodically sends timestamp packets to the slaves

* The slave evaluates the instantaneous timestamp difference by subtracting the received master timestamp from its local timestamp

* The slave computes the statistics of the timestamp difference on an observation window of fixed size

* At the end of each observation window, the slave corrects its clock by the delta between the timestamp difference statistics and the
  estimated IP channel median latency.

It is worth noting the master timestamp sending is not exactly periodic, it is quasi-periodic. The timestamp packet transmission is
staggered by a random amount of time in order avoid alignment with potential periodic channel behaviors.

During sychronization, if the estimated error is below the 'time correction step threshold', the slave clock is adjusted monotonically
acting on its frequency. If the estimated error is above such threshold, then the slave clock time is changed abruptly and may also
go back in time.

## Short guide

The following sections reports a small guide on how to use the two programs provided with this project, i.e.  `pspm` (PSP Master) and
`psps` (PSP Slave). For a detailed description of programs options, please refer to the man pages.

### Pre-Calibration

PSP pre-calibration can be performed using the following commands:

* on the slave: `psps -a -w 480 -d`

* on the master: `pspm -a <slave IP address> -d 250 -s 100 -v 3`

With such parameters:

* the synchronization master sends 4 timestamp per second on average.

* the synchronization slave estimates the relative frequency offset based on the median of channel latency computed over observation
  windows of 2 minutes. The first frequency offset estimation is produced after 4 minutes and then it is refreshed every 2 minutes.

* the synchronization slave produces two debug files: 'precalibr\_timestamp.txt' and 'precalibr\_freq\_delta.txt'. The former
  reports, for each received timestamp packet, the timestamps count, the slave clock time, the timestamp time and the difference
  between the two. The latter file reports the frequency offset estimated after each observation window (excluding the very
  first one).

Note: pre-calibration can be skipped if the master and slave clocks are synchronized continuously or if both clocks are known to
have a very low frequency offset.

### Calibration

In order to perform the calibration step, master and slaves machines must be synchronized to the same time reference through other
means. This can be achieved with GNSS, NTP or PTP protocols.

Once the master and slave machines are synchronize to the same time reference, PSP calibration can be performed using the following
commands:

* on the slave: `psps -c -w 7200 -d`

* on the master: `pspm -a <slave IP address> -d 250 -s 100 -v 3`

With such parameters:

* the synchronization master sends 4 timestamp per second on average.

* the synchronization slave estimates the IP channel median latecy over an observation windows of 30 minutes

* the synchronization slave produces two debug files: 'calibr\_timestamp.txt' and calibr\_time\_delta\_cdf.txt'. The former
  reports, for each received timestamp packet, the timestamps count, the slave clock time, the timestamp time and the difference
  between the two. The latter file reports the estimated Cumulative Distribution Function (CDF) of the IP channel latency.

### Synchronization

In this step, the master and slave shall not be synchronized through other means/protocols.

Slave can be synchronized to slave machine using the following commands:

* on the slave: `psps -s -w 240 -q 3 -d`

* on the master: `pspm -a <slave IP address> -d 250 -s 100 -v 3`

With such parameters:

* the synchronization master sends 4 timestamp per second on average.

* the synchronization slave adjusts/corrects its is clock based on measurements performed over increasing observation window size.
  The first correction is performed after 1 minute, the second one after additional 2 minutes, the third one after additional 4 minutes,
  the forth one after additional 8 minutes. The next corrections are then performed every 8 minutes.

* the synchronization slave produces three debug files: synch\_timestamp.txt', 'synch\_time\_correction.txt' and 
  'synch\_time\_cumul\_correction.txt'. The first one reports, for each received timestamp packet, the timestamps count, the slave clock 
  time, the timestamp time and the difference between the two. The second file reports the adjustments/corrections performed on the
  slave clock. The third file reports the cumulative of the adjustments/corrections performed on the slave clock.

### Early termination

Both synchronization master and slave can be stopped at any time pressing Ctrl-C. If the synchronization slave is stopped in this way
during pre-calibration or calibration, it saves the estimations performed so far in the result files.
