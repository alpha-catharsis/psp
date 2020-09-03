# Packet Synchronization Protocol (PSP)

## Important Notice

At the current stage this project can be considered as a proof-of-concept. It works but it is very basic and requires some manual tuning
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
statistics of the IP communication channel. For this reason PSP is based on a two-step approach: _calibration_ step and _synchronization_ step.

The calibration step allows the slave to estimate the IP channel latency statistics. In this step the master and slave must be synchronized to a
common time through other means (e.g. GNSS or NTP). During calibration step the master periodically sends timestamp packets to the slave, which
measures the channel latency statistics.

Once calibration has been performed, it is possible to disconnect the master and slave from the common time source and proceed with the
synchronization through PSP protocol. During this step, the synchronization master periodically sends timestamp packets to the slave that,
based on recorded channel latency statistics, corrects its clock.

It is worth noting that the master always performs the same operation (i.e. timestamp packet transmission) during both calibration and
synchronization steps.

The accuracy of PSP protocol strongly depends on the calibration error and on the stability of channel latency statistics over time. Calibration
error depends on the accuracy of the master/slave synchronization during calibration and on the time spent during calibration for the
estimation of channel latency statistics. The stability of the channel latency statistics may vary significantly based on the network topology and
media. Best stability is achieved on low-loaded LANs and Wifi networks, while on internet the stability is much less predictable.

From a limited set of experiments it results that very good performance is achievable on wired LANs (order of tens of microseconds), and that decent
performance is achievable through internet (milliseconds error).

## Installation

PSP depends only on the availability of C standard library and POSIX realtime extension library. It is compatible with POSIX.1c and more recent
systems. In order to install PSP the following commands shall be executed from a POSIX shell:
~~~~
tar -xzvf psp-1.1.1.tar.gz
cd psp-1.1.1
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

During calibration:

* Master and slave are synchronized with good accuracy to a common time reference through other means

* Master periodically sends timestamp packets to the slave

* Slave measures the channel latency subtracting the received master timestamp from its local timestamp

At the end of the calibration step, the slaves save in a local file the measured channel latency statistics in terms of mean value, 10th
percentile, 25th percentile and median.

During synchronization:

* The master and the slave are not synchronized to the a common time reference through other means

* The master periodically sends timestamp packets to the slaves

* The slave evaluates the instantaneous timestamp difference by subtracting the received master timestamp from its local timestamp

* The slave computes the statistics of the timestamp difference on an observation window of fixed size

* At the end of each observation window, the slave corrects its clock by the delta between the timestamp difference statistics and the
  recorded channel latency statistics. Any of the recorded statistics (mean, 10th percentile, 25th percentile or median) can be used for
  this purpose.

It is worth noting the master timestamp sending is not exactly periodic, it is quasi-periodic. The timestamp packet transmission is
staggered by a random amount of time in order avoid alignment with potential periodic channel behaviors.

At the moment the slave clock is adjusted abruptly at the end of each observation window, fully compensating the estimated synchronization
error. This may result in the slave clock going back in time. In the future, the adjustment of slave clock frequency might be considered
to guarantee a monotonic slave clock time.

## Short guide

The following sections reports a small guide on how to use the two programs provided with this project, i.e.  `pspm` (PSP Master) and
`psps` (PSP Slave). For a detailed description of programs options, please refer to the man pages.

### Calibration

In order to perform the calibration step, master and slaves machines must be synchronized to the same time reference through other
means. This can be achieved with GNSS, NTP or PTP protocols.

It is important to distinguish two cases:

1. the master and slaves are continuously synchronized to the same time reference. In this case the relative drift between master
   and slaves clocks can be probably considered as negligible

2. Master and slave clocks synchronization to the same time reference is performed only once before performing the calibration step.
   In this case the relative drift between master and slave clocks can be relevant or not depending on the clock quality and on
   the target synchronization error that must be achieved

Once the master and slave machines are synchronize to the same time reference, PSP calibration can be performed using the following
commands:

* on the slave: `psps -c -u stats.txt -n <numer of samples for calibration> -d <length of drift evaluation window> -t lat.txt -v 3`

* on the master: `pspm -a <slave IP address> -d <timestamp transmission period in ms> -s <stagger in ms> -v 3`

The number of samples for calibration should be as high as possible. A good starting value ranges between 5000 and 10000.
The length of the drift evaluation window should range between 50 and 200.
The timestamp transmission period can be set to 1000 (1 second) but, if the channel allows it, a value of 100 (100 milliseconds)
would significantly speed up the calibration/synchronization processes.

Note: both master and slave can be stopped at any time pressing Ctrl-C. If slave is stopped in this way, it saves the channel
latency statistics measured up to that point.

At the end of calibration is important to verify the value `cumul_drift` reported in the `stats.txt` file. If this number is
not an order of magnitude lower that the desired synchronization error, then it is necessary to re-run the calibration by
specifying also the option `-i <drift_ppb value reported in file stats.txt>`.

Once calibration has been performed, it may be interesting to have a look at the file "lat.txt" that contains the series of measured
latency samples. This can be done, for example, with gnuplot using the following command: `gnuplot -e "plot 'lat.txt'" -p`.

### Synchronization

Once calibration is complete, slave can be synchronized to slave machine using the following commands:

* on the slave: `psps -s -u stats.txt -w <number of observation window samples> -a <algorithm>`

* on the master: `pspm -a <slave IP address> -d <timestamp transmission period in ms> -s <stagger in ms> -v 3`

It is important to use for the master the same timestamp transmission period and stagger utilized during calibration.

The number of observation window sample must be selected based on the stability of the IP channel. A good starting value
ranges between 100 and 1000. This number, together with the master timestamp transmission period, determines how often
the slave clock is adjusted, i.e. `<slave clock adjustment period> = <timestamp transmission period> * <number of observation
window samples>`. For example, if the timestamp transmission period is 100 ms and the observation window size is 600, the
slave clock is adjuster roughly once per minute.

The flag `-a` is used to select the algorithm. At the moment the most promising algorithm seems the one based on **median**
latency statistics, that can be selected with parameter `-a 3`.

When slave clock is adjusted, psps prints the correction applied to the clock. For sufficiently long observation window,
this correction has the same order of magnitude of the synchronization error, excluded the calibration error that cannot
be estimated in any way by the slave process.

