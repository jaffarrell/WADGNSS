This repository provides open-source software and documentation that implements the client portion of a client-server architecture for computing common-mode GPS pseudorange error corrections and communicating them to a receiver. The information for the corrections is imported by the server from information sources available in real-time over the internet. The server accesses that information, computes the common-mode error correction for a client specified location, and communicates the correction to the receiver using the NTRIP protocol with RTCM message types 1004 and 1005. 

The client software released here communicates between the user receiver and the correction server. The client establishes connections with the recevier and the server. Then, the client (1) communicates the users desired reference station location to the server, (2) communicates the server messages to the receiver. The client source code can be modified by the user to send the receiver output messsages to other applications or computer output ports.


## Contributors

* Wang Hu - [GitHub](https://github.com/Azurehappen)
* Dan Vyenielo - [GitHub](https://github.com/dvnlo)
* Farzana Rahman - [GitHub](https://github.com/FarzanaRahman)
* Xiaojun Dong - [GitHub](https://github.com/Akatsukis)
* Jay Farrell - [GitHub](https://github.com/jaffarrell)
