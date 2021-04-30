# LibSEAL

LibSEAL is a SEcure Auditing Library for internet services. It allows to
detect integrity violations of internet services without the need to
trust the service operator. To do so, LibSEAL: (i) constructs a secure
log of requests and responses exchanged between clients and the internet
service; and (ii) periodically checks the log for integrity violations.
Technically, LibSEAL combines [TaLoS](https://github.com/lsds/TaLoS), a
secure TLS communication library, with [SQLite](https://www.sqlite.org)
in order to create and check logs. Further details about the operation
of LibSEAL are described in the corresponding [EuroSys'18
publication](https://lsds.doc.ic.ac.uk/content/libseal-revealing-service-integrity-violations-using-trusted-execution).

This repository contains the source code of LibSEAL. It ships as a set
of patches on top of TaLoS. In particular, the repository ships with
auditing modules for the Dropbox, Git and ownCloud services.

## Compilation and installation

LibSEAL builds on top of the TaLoS library. You thus need to clone both
the TaLoS and the LibSEAL repositories:
```bash
$ git clone https://github.com/lsds/TaLoS
$ git clone https://github.com/lsds/LibSEAL
```

We assume that you have cloned TaLoS into the `${TALOS_ROOT}` directory
(e.g. `/home/<username>/talos/`) and that you have cloned LibSEAL into
the `${LIBSEAL_ROOT}` directory (eg `/home/<username>/libseal/`).

You first need to copy the LibSEAL patches into the TaLoS source directory:
```bash
$ cd ${TALOS_ROOT}/src
$ cp -r ${LIBSEAL_ROOT}/src/libseal .
$ cd talos && ./patch_libressl.sh && cd ..
$ cd libseal && ./patch_talos.sh && cd ..
```

After that, please follow the instructions in the [TaLoS readme
file](https://github.com/lsds/TaLoS/blob/master/README.md). Since you applied the
LiBSEAL patches above, this will indeed compile LibSEAL.

By default, LibSEAL uses the Git auditing module. To use LiBSEAL with
modules (ownCloud or Dropbox), you need to change the
`enclave.signed.so` symlink and the `auditing.so` symlink to point to
the appropriate auditing module in directory
`${TALOS_ROOT}/src/libressl-2.4.1/crypto`.

## Auditing modules

LibSEAL ships with three auditing modules, for Dropbox, Git and ownCloud.
These are located in directory
`${TALOS_ROOT}/src/libressl-2.4.1/crypto/auditing`.

Each module defines:
- Code to process the service's HTTP requests/responses, extracting
information to create the logs that will be checked for integrity
violations;
- SQL queries to check for integrity violations. These queries return an empty set only if no violations have been detected. The definition
of an integrity violation depends on the service. For example, for
Dropbox, LibSEAL detects whether the list of files provided by Dropbox
corresponds to the list of files that the client has uploaded;
- SQL queries to trim the log, i.e., to remove entries that are no
longer needed to check the log. Trimming queries are used to reduce the
log size.

## Monotonic counter service

LibSEAL can use a distributed monotonic counter service to prevent
rollback attacks. The implementation (files `mcservice.c` and
`mcservice.h` in directory
`${LIBSEAL_ROOT}/src/monotoniccounterservice`) is similar to the service
described in the [ROTE](https://www.usenix.org/system/files/conference/usenixsecurity17/sec17-matetic.pdf) paper.

To use this service, you first need to define the
`USE_MONOTONIC_COUNTER_SERVICE` macro in `enclaveshim_config.h`. Then,
you need to create a `monotonic_counter_service.txt` file that has to be
accessible by your application (see the `mcservice_initialize()`
function in `mcservice.c`). This file must contain the IP address of
every machine that will be used for the monotonic counter service. These
machines must start the server present in the
`${LIBSEAL_ROOT}/src/monotoniccounterservice` directory. Two scripts, to
start and stop a server, are provided in this directory.

Upon startup, LibSEAL will connect to these servers. It will then
exchange (encrypted) messages for each database insert in order to
increment the monotonic counter.

## Controlling LibSEAL behaviour

LibSEAL adds several new macros in the `enclaveshim_config.h` file:
- `DO_LOGGING`: define this macro if you want to use the logging module;
- `LOG_FOR_SQUID`: define this macro if you are using Squid. This is to
avoid logging both the messages transmitted between the client and the
proxy and the messages transmitted between the proxy and the server;
- `USE_MONOTONIC_COUNTER_SERVICE`: define this macro to use the distributed
monotonic counter service. See the previous section for more details;
- `SQLITE_DB_NAME`: this macro defines the path and name of the SQLite
database that contains the log  or `:memory:` for an in-memory database;
- `SQLITE_DO_INSERT`: define this macro to execute the database
insertions. This macro was used in our experimental evaluation to
measure the cost of the processing of HTTP messages without any database
operation;
- `SQLITE_ASYNC_MODE`: define this macro to write the database
asynchronously to disk;
- `SQLITE_ADD_HASH`: define this macro to add a hash to every database
entry, for integrity;
- `SQLITE_DO_SIGN`: define this macro to add a signature to every
database entry, for integrity;
- `SQLITE_DO_CHECK`: define this macro to periodically check for
invariant violations;
- `SQLITE_CHECKAFTER`: this macro controls the period, in terms of
number of entries inserted into the database, at which the SQL queries
for revealing invariant violations are executed;
- `SQLITE_DO_TRIMMING`: define this macro to execute the trimming query,
in order to remove from the log entries that are no longer necessary.
