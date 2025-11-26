## Performance Test for Fapolicyd

The code in this directory can be used to performance test the decision thread. The way that the test suite is constructed only allows testing the object side of the rules. This is because it is difficult to simulate a lot of subjects from a test driver. This performance test uses the kernel's perf utility to record information. So, here are the steps on a Fedora system:

```
dnf install perf

Install from updates to get the latest:
dnf install -y --enable-repo=updates-debuginfo rpm-libs-debuginfo \
rpm-debuginfo  lmdb-debuginfo glibc-debuginfo glibc-common-debuginfo \
openssl-libs-debuginfo libattr-debuginfo file-debuginfo --skip-unavailable

Install anything missing from fedora
dnf install -y --enable-repo=fedora-debuginfo rpm-libs-debuginfo \
rpm-debuginfo lmdb-debuginfo glibc-debuginfo glibc-common-debuginfo \
openssl-libs-debuginfo libattr-debuginfo file-debuginfo --skip-unavailable

```

For security purposes, fapolicyd's intenal library is built for static linking. So, to run this test suite, it needs to find an unlinked copy of that library. It doesn't matter if it was an srpm or built from a freshly cloned github repo.
Adjust the path in the Makefile so that it can find the freshly built fapolicyd. The first fix is to point the includes (-I) to the fapolicyd header files. There are 2 sets, the config.h file which is in the top most directory and then the library headers. The second fix is to point the linker to the fapolicyd library (-L). For this example, we will assume the user is in a "build" account. Adjust accordingly.

```
vi Makefile
make

rm -f file-list.txt
touch file-list.txt
find /usr/lib64 -type f >> file-list.txt
find /usr/bin   -type f >> file-list.txt
find /usr/share -type f >> file-list.txt

su root    # Need to be root to access the config files
./test-driver
perf report

```
The test driver uses a list of files to access. The list can be populated any way you like. You just need a lot of them. The test driver will open file-list.txt and create a bunch of decision requests from the list.

The test driver has a string inside it called cmd_template. It looks like this:

```
/usr/bin/perf record --call-graph dwarf --no-inherit -p %d &
```

You can modify that any way you like. What this does is launch the perf command in the record mode and collecting a dwarf style call graph. It does not want any child processes and points to the test driver's pid. It runs in the background so that the test can proceed.

After collection, you can process the captured data any way you want. In the example above, it runs the interactive perf report. You can also take the data and turn it into a flame-graph. You can get the code needed below from here: https://github.com/brendangregg/FlameGraph. Put the scripts in a directory in your path. The run this:

```
chown build perf.data
exit  # go back to normal user
perf script | stackcollapse-perf.pl --all | flamegraph.pl > fapolicyd.svg

firefox file://`pwd`/fapolicyd.svg
```

Be sure to zoom in a lot. You can click on a function to zoom into its call grapgh. Read up on how to interpret flame graphs. They are not what you think they are until to you read how they work.
