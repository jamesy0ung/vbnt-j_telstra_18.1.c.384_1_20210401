
AllJoyn SDK for Android
-----------------------

This subtree contains one complete copy of the AllJoyn SDK for Android, built
for a single CPU (either x86 or arm) and VARIANT (either debug or release).
The CPU and VARIANT are normally incorporated into the name of the package or
folder containing this SDK.

Please see ReleaseNotes.txt for the applicable AllJoyn release version and
related information on new features and known issues.


Summary of file and directory structure:
----------------------------------------

The contents of this SDK are arranged into the following top level folders:

cpp/    core AllJoyn functionality, implemented in C++
          - built from alljoyn.git source subtrees alljoyn_core and common
          - required for all AllJoyn applications
java/   optional Java language binding          (built from alljoyn_java)
          - required for Android apps
c/      optional ANSI C language binding        (built from alljoyn_c)
about/  implements AllJoyn About Feature. (built from services/about/(cpp and java))


The contents of each top level folder are further arranged into sub-folders:

        ----------------------------------------------
cpp/    core AllJoyn functionality, implemented in C++
        ----------------------------------------------

    bin/                        executable binaries
                                  - as of 3.4.0, several test programs are
                                    intentionally removed from published SDK's

        alljoyn-daemon                  installable AllJoyn daemon, for OEMs
                                          - or devices with super-user access

    docs/html/                  AllJoyn Core API documentation

    inc/alljoyn/                AllJoyn Core (C++) headers
    inc/qcc/

    lib/                        AllJoyn Core (C++) client libraries

        liballjoyn.a                    implements core API
        libajrouter.a                   implements bundled-router
        liballjoyn-daemon.a
        libbbdaemon-lib.a

    samples/                    C++ sample apps for Android (see README)


        ---------------------
java/   Java language binding
        ---------------------

    docs/html/                  API documentation

    jar/                        client library, misc jar files

        alljoyn.jar                     AllJoyn Java API

    lib/liballjoyn_java.so      client library

    samples/                    sample apps for Android (see README)


        -----------------------
c/      ANSI C language binding
        -----------------------

    docs/html/                  API documentation

    inc/alljoyn_c/              ANSI C headers
    inc/qcc/

    lib/                        client libraries


        ---------------------
about/  AllJoyn About Feature
        ---------------------

    jar/
        alljoyn_about.jar               AllJoyn About Java API

