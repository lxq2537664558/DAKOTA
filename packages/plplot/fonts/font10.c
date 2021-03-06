/* $Id: font10.c 3186 2006-02-15 18:17:33Z slbrow $
   $Log$
   Revision 1.1  2006/02/15 18:15:44  slbrow
   This is the commit of the DAKOTA 'autoconfiscation' I've been working on, which
   enables builds using the GNU Autotools.  I have been merging in changes to the
   mainline the entire time I've been working on these changes to my working copy
   (in ~slbrow/projects/Dakota), so none of your changes to DAKOTA will be lost.
   In addition, the final pre-autotools version of DAKOTA has been tagged with:

   Version-3-3-Plus-pre-autotools

   The GNU autotools will be needed by all developers who check DAKOTA out from
   CVS with intentions to build; this is because the version of DAKOTA in the
   repository after this commit does not have any configure or Makefile.in files
   (since they are now machine-generated).  All the source tarballs rolled for
   external consumption (both release and votd) will automatically include all the
   configure and Makefile.in files our non-autotools savvy users will need, while
   still omitting those proprietary sources we may not distribute (DOT, NPSOL).

   To re-generate the missing configure and Makefile.in files in a checkout of
   DAKOTA, only one GNU Autotools script is necessary to run from the
   command-line:

   $ autoreconf --install

   This script (and all the scripts it runs in sequence on a copy of DAKOTA
   checked out of the repository) can be found on the SCICO LAN in the directory:

   /usr/netpub/autotools/bin

   This directory exists on all five of the DAKOTA nightly build platforms (AIX,
   IRIX, Linux, Solaris and Tru64), and should be added to the beginning of your
   $PATH if you are on the SCICO LAN.  If you are not, the versions of the tools
   you need can be easily downloaded from the GNU website.  The versions we use:

   autoconf 2.59  (http://ftp.gnu.org/gnu/autoconf/autoconf-2.59.tar.gz)
   automake 1.9.6 (http://ftp.gnu.org/gnu/automake/automake-1.9.6.tar.gz)
   libtool 1.5.22 (http://ftp.gnu.org/gnu/libtool/libtool-1.5.22.tar.gz)
   m4 1.4.3       (http://ftp.gnu.org/gnu/m4/m4-1.4.3.tar.gz)

   Here is a "short" summary of the major changes made in this )commit:

   1. All configure.in and Makefile.in files are no longer in the CVS repository.
      They are replaced by configure.ac and Makefile.am files, respectively.  When
      you run autoreconf, the configure and Makefile.in files are regenerated
      automatically and recursively.  No $DAKOTA environment variable needs to be
      set in order to build DAKOTA.

   2. Build directories as they were in DAKOTA using the Cygnus configure system
      no longer exist.  Executables built by the GNU Autotools (e.g., the dakota*
      binaries which were built in Dakota/src/i686-unknown-linux on Linux) are now
      built in Dakota/src; the autotools provide deployment targets for installing
      binaries and libraries in a deployment directory, as well as a simple means
      of maintaining more than one build of Dakota for multiple platforms (i.e.,
      using VPATH builds and source- and build-tree separation).

      Suppose you have in your home directory a checked-out and 'autoreconf'd copy
      of Dakota, and you want to simultaneously build versions of DAKOTA for Linux
      and Solaris.  You would create two uniquely named directories in your home
      directory, called 'my_linux_build' and 'my_solaris_build', respectively.
      Assuming two terminals are open on both machines, you would 'cd' into the
      appropriate build directory on each platform and run this command on both:

      $ ../Dakota/configure -C

      This will create a directory tree which is identical to the source tree, but
      containing only Makefiles.  Then, when you run 'make', it will traverse the
      build tree, referencing the sources by relative paths to create objects and
      binaries.  At your option, you can then delete these build directories
      without touching your source tree (the 'autoreconf'd checkout of Dakota).  I
      should note here that the GNU Autotools do not _force_ this separation of
      trees; you can do a 'make distclean' after building for each platform in
      your 'autoreconf'd copy of Dakota with no problems.

   3. All the configure flags which were supported in the old build system (i.e.,
      --without-acro, --enable-debugging, etc.), are supported in the new system
      with the following changes and additions:

      --with-mpi/--without-mpi           -> --enable-mpi/--disable-mpi
      --with-graphics/--without-graphics -> --enable-graphics/--disable-graphics
      --enable-docs (default is off)

      I should note that all the default settings have been preserved.  Further,
      since ACRO, OPT++ and DDACE haven't been integrated yet (see item 9), their
      flags are still commented out in 'Dakota/config/dak_vendoroptimizers.m4'.

   4. The epetra and plplot directories under VendorPackages have been completely
      replaced with the versions available online (versions 3.3 and 5.5.2,
      respectively), which were originally flattened to provide support for our
      build system.  Epetra has been altered from the version distributed by the
      Trilinos team to provide full libtool support as that is necessary for
      building mixed-language archives under Solaris, while plplot has had all
      language-bindings except C++ turned off by default.  Additionally, GSL has
      been updated from version 1.7 available online, but preserving the flattened
      structure it had prior to autoconfiscation.

   5. All other packages in the DAKOTA repository under VendorPackages and
      VendorOptimizers have been 'autoconfiscated' separately from DAKOTA.  To
      avoid duplicated configure-time probes, run configure from the top-level
      DAKOTA directory with the '-C' argument; this will cache the probe results
      from each subpackage, removing the need to do anything other than a lookup
      the next time the probe runs.

   6. No 'mpi' symbolic link needs to be made from VendorPackages/mpi to the MPI
      implementation you wish to build against; MPI wrapper scripts for the C++
      compiler are used instead; if you wish to change the default choices for the
      MPI implementation to build against, you can specify where the wrapper
      scripts you wish to use are via environment variables, which may be set
      on separate lines (using setenv in CSH), or with env on the configure line:

      $ env MPICC=mpicc MPICXX=mpiCC MPIF77=mpif77 ./configure -C

   7. All the clean-up make targets ('clean', 'distclean' and a new one for those
      unfamiliar with the GNU Autotools, 'maintainer-clean') are now machine-
      generated.  The targets remove increasing numbers of files as you would
      expect; the last one removes all the autotools-generated files, returning
      you to a pristine CVS checkout.

   8. Many changes have been made to the sources in Dakota/src:

      A) Fortran calls are now done using FC_FUNC and F77_FUNC macro wrappers,
         removing the need for #ifdef RS6K preprocessor checks for underscores.
      B) Platform testing preprocessor conditionals have been replaced* with the
         feature that was assumed to be supported on that platform 'a priori'
         * mostly: MPI implementation choice is still done by platform.
           See ParallelLibrary.C for details.
      C) TFLOPS and COUGAR #ifdef's have been removed, leaving their #else
         branches to execute unconditionally (as those machines have gone away).

   9. Since the GNU Autotools preferentially look for GCC compilers first; if you
      wish to build on Solaris with the Sun compilers you must set the compilers
      to use in environment variables in the same way the MPI wrappers must be set
      (see item 5 above):

       $ env CC=cc CXX=CC F77=f77 FC=f90 ./configure ...

   Additionally, there are some minor problems to fix with the new build system:

   10. This autoconfiscation covers Dakota, as integration with ACRO, OPT++ and
       DDACE has yet to be done.  I have integrated Surfpack and removed the
       separate ann and kriging packages from VendorPackages, based on positive
       results from some integration testing I've done on the Surfpack versions of
       ann and kriging against those in the separate versions in VendorPackages.

   11. Purely static-linked executables have proved difficult with libtool.  The
       way libtool sets things up, all the DAKOTA package libraries are static and
       linked as such, but system libraries are linked against dynamically (the
       way it is currently done on Solaris).  Strangely though, the MPI libraries
       aren't linked against dynamically, so this is an area for investigation
       (especially as I planned on having it working for the ASC White/Purple
       deployment of Dakota 4.0).

   12. Running 'make -j <n>' with positive integer n for parallel builds is not
       yet supported, as there are a couple packages with dependency requirements
       (LHS and IDR, namely) which preclude compiling in parallel until their
       requirements are met.  In LHS, those requirements arise from the need for
       all the *.mod module files to exist prior to anything which USE's them, and
       in IDR, they arise from the lex/yacc generated source dependencies.

   13. Disabling F90 support (via the --disable-f90 configure flag) doesn't work
       yet.  GNU Automake isn't quite smart enough yet to accept arbitrary
       suffixes for F90 files, so it expects that the suffixes will be .f90 or
       .f95, _not_ .F!  The Automake developers are working on this, so in the
       meantime, I'm using a hack.  I put that hack into Dakota/src/Makefile.am to
       address this situation (the same one I use in LHS, by the way, which sets
       F77 and FFLAGS to their F90-equivalents) doesn't work when F90 is disabled.

   14. The nightly build and test system (in Dakota/test/sqa) will need to be
       modified to use the new system, so the nightlies will be broken until I can
       get the scripts modified to use the new system.  That will be my next CVS
       checkin, and I don't expect it to take very long to make those mods.

   Revision 1.1  1992/05/20 21:33:31  furnish
   Initial checkin of the whole PLPLOT project.

*/

      short int subbuffer225[100] = {
       8511, 8508, 8378, 7992, 7735,   64, 8256, 8383, 8380, 8250,
       7992,   64, 7735, 7990, 8116, 8113, 7982, 7724, 7467, 7340,
       7342, 7473, 7860, 8246, 8761, 9148,   64, 7735, 7862, 7988,
       7985, 7854, 7724,    0,  -16, 7116, 8256,   64,    0,  -16,
       7116, 7748,   64,    0,  -16, 7116, 7240,   64,    0,  -16,
       7116, 6986, 8140, 7755, 7496, 7363, 7360, 7483, 7736, 8119,
       8375, 8760, 9019, 9152, 9155, 9032, 8779, 8396, 8140,   64,
       7883, 7624, 7491, 7488, 7611, 7864,   64, 7737, 8120, 8376,
       8761,   64, 8632, 8891, 9024, 9027, 8904, 8651,   64, 8778,
       8395, 8139, 7754,    0,  -16, 7116, 6986, 7752, 8009, 8396
      };
      short int subbuffer226[100] = {
       8375,   64, 7752, 7751, 8008, 8266, 8247, 8375,    0,  -16,
       7116, 6986, 7495, 7496, 7626, 7755, 8012, 8524, 8779, 8906,
       9032, 9030, 8900, 8641, 7479,   64, 7495, 7623, 7624, 7754,
       8011, 8523, 8778, 8904, 8902, 8772, 8513, 7351,   64, 7480,
       9144, 9143,   64, 7351, 9143,    0,  -16, 7116, 6986, 7628,
       9036, 8131,   64, 7628, 7627, 8907,   64, 8908, 8003,   64,
       8132, 8388, 8771, 9025, 9150, 9149, 9018, 8760, 8375, 7991,
       7608, 7481, 7355, 7483,   64, 8003, 8387, 8770, 9023,   64,
       8515, 8897, 9022, 9021, 8890, 8504,   64, 9020, 8761, 8376,
       7992, 7609, 7483,   64, 7864, 7482,    0,  -16, 7116, 6986
      };
      short int subbuffer227[100] = {
       8649, 8631, 8759,   64, 8780, 8759,   64, 8780, 7356, 9276,
         64, 8649, 7484,   64, 7485, 9277, 9276,    0,  -16, 7116,
       6986, 7628, 7491,   64, 7755, 7620,   64, 7628, 8908, 8907,
         64, 7755, 8907,   64, 7620, 8005, 8389, 8772, 9026, 9151,
       9149, 9018, 8760, 8375, 7991, 7608, 7481, 7355, 7483,   64,
       7491, 7619, 7876, 8388, 8771, 9024,   64, 8516, 8898, 9023,
       9021, 8890, 8504,   64, 9020, 8761, 8376, 7992, 7609, 7483,
         64, 7864, 7482,    0,  -16, 7116, 6986, 8779, 8905, 9033,
       8907, 8524, 8268, 7883, 7624, 7491, 7486, 7610, 7864, 8247,
       8375, 8760, 9018, 9149, 9150, 9025, 8771, 8388, 8260, 7875
      };
      short int subbuffer228[100] = {
       7617,   64, 8906, 8523, 8267, 7882,   64, 8011, 7752, 7619,
       7614, 7738, 8120,   64, 7612, 7865, 8248, 8376, 8761, 9020,
         64, 8504, 8890, 9021, 9022, 8897, 8515,   64, 9023, 8770,
       8387, 8259, 7874, 7615,   64, 8131, 7745, 7614,    0,  -16,
       7116, 6986, 7372, 9164, 7863,   64, 7372, 7371, 9035,   64,
       9036, 7735, 7863,    0,  -16, 7116, 6986, 8012, 7627, 7497,
       7495, 7621, 7748, 8003, 8514, 8769, 8896, 9022, 9019, 8889,
       8504, 7992, 7609, 7483, 7486, 7616, 7745, 8002, 8515, 8772,
       8901, 9031, 9033, 8907, 8524, 8012,   64, 7755, 7625, 7623,
       7749, 8004, 8515, 8770, 9024, 9150, 9147, 9017, 8888, 8503
      };
      short int subbuffer229[100] = {
       7991, 7608, 7481, 7355, 7358, 7488, 7746, 8003, 8516, 8773,
       8903, 8905, 8779,   64, 8906, 8523, 8011, 7626,   64, 7482,
       7864,   64, 8632, 9018,    0,  -16, 7116, 6986, 8898, 8640,
       8255, 8127, 7744, 7490, 7365, 7366, 7497, 7755, 8140, 8268,
       8651, 8905, 9029, 9024, 8891, 8632, 8247, 7991, 7608, 7482,
       7610, 7736,   64, 8901, 8770, 8384,   64, 8900, 8641, 8256,
       8128, 7745, 7492,   64, 8000, 7618, 7493, 7494, 7625, 8011,
         64, 7495, 7754, 8139, 8267, 8650, 8903,   64, 8395, 8777,
       8901, 8896, 8763, 8504,   64, 8633, 8248, 7992, 7609,    0,
        -16, 7116, 7622, 8250, 8121, 8120, 8247, 8375, 8504, 8505
      };
      short int subbuffer230[100] = {
       8378, 8250,   64, 8249, 8248, 8376, 8377, 8249,    0,  -16,
       7116, 7622, 8504, 8375, 8247, 8120, 8121, 8250, 8378, 8505,
       8502, 8372, 8115,   64, 8249, 8248, 8376, 8377, 8249,   64,
       8375, 8502,   64, 8504, 8372,    0,  -16, 7116, 7622, 8261,
       8132, 8131, 8258, 8386, 8515, 8516, 8389, 8261,   64, 8260,
       8259, 8387, 8388, 8260,   64, 8250, 8121, 8120, 8247, 8375,
       8504, 8505, 8378, 8250,   64, 8249, 8248, 8376, 8377, 8249,
          0,  -16, 7116, 7622, 8261, 8132, 8131, 8258, 8386, 8515,
       8516, 8389, 8261,   64, 8260, 8259, 8387, 8388, 8260,   64,
       8504, 8375, 8247, 8120, 8121, 8250, 8378, 8505, 8502, 8372
      };
      short int subbuffer231[100] = {
       8115,   64, 8249, 8248, 8376, 8377, 8249,   64, 8375, 8502,
         64, 8504, 8372,    0,  -16, 7116, 7622, 8268, 8254, 8382,
         64, 8268, 8396, 8382,   64, 8250, 8121, 8120, 8247, 8375,
       8504, 8505, 8378, 8250,   64, 8249, 8248, 8376, 8377, 8249,
          0,  -16, 7116, 7114, 7495, 7496, 7626, 7755, 8140, 8524,
       8907, 9034, 9160, 9158, 9028, 8899, 8642, 8257,   64, 7495,
       7623, 7624, 7754, 8139, 8523, 8906, 9032, 9030, 8900, 8643,
       8258,   64, 7625, 8011,   64, 8651, 9033,   64, 9029, 8514,
         64, 8258, 8254, 8382, 8386,   64, 8250, 8121, 8120, 8247,
       8375, 8504, 8505, 8378, 8250,   64, 8249, 8248, 8376, 8377
      };
      short int subbuffer232[100] = {
       8249,    0,  -16, 7116, 7622, 8524, 8267, 8137, 8134, 8261,
       8389, 8518, 8519, 8392, 8264, 8135,   64, 8263, 8262, 8390,
       8391, 8263,   64, 8267, 8135,   64, 8137, 8264,    0,  -16,
       7116, 7622, 8522, 8393, 8265, 8138, 8139, 8268, 8396, 8523,
       8520, 8390, 8133,   64, 8267, 8266, 8394, 8395, 8267,   64,
       8393, 8520,   64, 8522, 8390,    0,  -16, 7116, 6733, 9541,
       9285, 9028, 8898, 8636, 8506, 8377, 8120, 7608, 7353, 7227,
       7229, 7359, 7488, 8131, 8389, 8519, 8521, 8395, 8140, 8012,
       7755, 7625, 7623, 7748, 8001, 8635, 9016, 9271, 9527,   64,
       9541, 9540, 9284, 9027,   64, 9156, 9026, 8764, 8634, 8376
      };
      short int subbuffer233[100] = {
       8119, 7607, 7352, 7225, 7099, 7101, 7231, 7489, 8132, 8261,
       8391, 8393, 8267,   64, 8394, 8139, 8011, 7754,   64, 7883,
       7753, 7751, 7876, 8129, 8763, 9017, 9272, 9528, 9527,    0,
        -16, 7116, 7114, 8272, 8243, 8371,   64, 8272, 8400, 8371,
         64, 8905, 9161, 8907, 8524, 8140, 7755, 7497, 7495, 7621,
       7748, 8768, 8895, 9021, 9019, 8889, 8504, 8120, 7865, 7738,
         64, 8905, 8778, 8523, 8139, 7754, 7625, 7623, 7749, 8769,
       9023, 9149, 9147, 9017, 8888, 8503, 8119, 7736, 7482, 7738,
         64, 9018, 8632,    0,  -16, 7116, 6860, 9424, 7088, 7216,
         64, 9424, 9552, 7216,    0,  -16, 7116, 7367, 8656, 8398
      };
      short int subbuffer234[100] = {
       8139, 7879, 7746, 7742, 7865, 8117, 8370, 8624, 8752,   64,
       8656, 8784, 8526, 8267, 8007, 7874, 7870, 7993, 8245, 8498,
       8752,    0,  -16, 7116, 7367, 7760, 8014, 8267, 8519, 8642,
       8638, 8505, 8245, 7986, 7728, 7856,   64, 7760, 7888, 8142,
       8395, 8647, 8770, 8766, 8633, 8373, 8114, 7856,    0,  -16,
       7116, 7240, 8268, 8139, 8385, 8256,   64, 8268, 8256,   64,
       8268, 8395, 8129, 8256,   64, 7625, 7753, 8771, 8899,   64,
       7625, 8899,   64, 7625, 7624, 8900, 8899,   64, 8905, 8777,
       7747, 7619,   64, 8905, 7619,   64, 8905, 8904, 7620, 7619,
          0,  -16, 7116, 6733, 7233, 9409, 9408,   64, 7233, 7232
      };
      short int subbuffer235[100] = {
       9408,    0,  -16, 7116, 6733, 8265, 8248, 8376,   64, 8265,
       8393, 8376,   64, 7233, 9409, 9408,   64, 7233, 7232, 9408,
          0,  -16, 7116, 6733, 7237, 9413, 9412,   64, 7237, 7236,
       9412,   64, 7229, 9405, 9404,   64, 7229, 7228, 9404,    0,
        -16, 7116, 7749, 8396, 8267, 8261,   64, 8395, 8261,   64,
       8396, 8523, 8261,    0,  -16, 7116, 7113, 7756, 7627, 7621,
         64, 7755, 7621,   64, 7756, 7883, 7621,   64, 8908, 8779,
       8773,   64, 8907, 8773,   64, 8908, 9035, 8773,    0,  -16,
       7116, 7367, 8140, 7883, 7753, 7751, 7877, 8132, 8388, 8645,
       8775, 8777, 8651, 8396, 8140,   64, 8140, 7753, 7877, 8388
      };
      short int subbuffer236[100] = {
       8775, 8651, 8140,   64, 8396, 7883, 7751, 8132, 8645, 8777,
       8396,    0,  -16, 7116, 8256,   64,    0,  -16, 7116, 7748,
         64,    0,  -16, 7116, 7240,   64,    0,  -16, 7116, 6987,
       8524, 8139, 7881, 7622, 7491, 7359, 7356, 7481, 7608, 7863,
       8119, 8504, 8762, 9021, 9152, 9284, 9287, 9162, 9035, 8780,
       8524,   64, 8524, 8267, 8009, 7750, 7619, 7487, 7484, 7609,
       7863,   64, 8119, 8376, 8634, 8893, 9024, 9156, 9159, 9034,
       8780,    0,  -16, 7116, 6987, 8520, 7863,   64, 8780, 7991,
         64, 8780, 8393, 8007, 7750,   64, 8649, 8135, 7750,    0,
        -16, 7116, 6987, 7880, 8007, 7878, 7751, 7752, 7882, 8011
      };
      short int subbuffer237[100] = {
       8396, 8780, 9163, 9289, 9287, 9157, 8899, 8513, 7999, 7613,
       7355, 7095,   64, 8780, 9035, 9161, 9159, 9029, 8771, 7999,
         64, 7225, 7354, 7610, 8248, 8632, 8889, 9019,   64, 7610,
       8247, 8631, 8888, 9019,    0,  -16, 7116, 6987, 7880, 8007,
       7878, 7751, 7752, 7882, 8011, 8396, 8780, 9163, 9289, 9287,
       9157, 8771, 8386,   64, 8780, 9035, 9161, 9159, 9029, 8771,
         64, 8130, 8386, 8769, 8896, 9022, 9019, 8889, 8760, 8375,
       7863, 7480, 7353, 7227, 7228, 7357, 7484, 7355,   64, 8386,
       8641, 8768, 8894, 8891, 8761, 8632, 8375,    0,  -16, 7116,
       6987, 9035, 8247,   64, 9164, 8375,   64, 9164, 7229, 9277
      };
      short int subbuffer238[100] = {
          0,  -16, 7116, 6987, 8140, 7490,   64, 8140, 9420,   64,
       8139, 8779, 9420,   64, 7490, 7619, 8004, 8388, 8771, 8898,
       9024, 9021, 8890, 8632, 8247, 7863, 7480, 7353, 7227, 7228,
       7357, 7484, 7355,   64, 8388, 8643, 8770, 8896, 8893, 8762,
       8504, 8247,    0,  -16, 7116, 6987, 9161, 9032, 9159, 9288,
       9289, 9163, 8908, 8524, 8139, 7881, 7622, 7491, 7359, 7355,
       7481, 7608, 7863, 8247, 8632, 8890, 9020, 9023, 8897, 8770,
       8515, 8131, 7874, 7616, 7486,   64, 8524, 8267, 8009, 7750,
       7619, 7487, 7482, 7608,   64, 8247, 8504, 8762, 8892, 8896,
       8770,    0,  -16, 7116, 6987, 7756, 7494,   64, 9420, 9289
      };
      short int subbuffer239[100] = {
       9030, 8384, 8125, 7995, 7863,   64, 9030, 8256, 7997, 7867,
       7735,   64, 7625, 8012, 8268, 8905,   64, 7754, 8011, 8267,
       8905, 9161, 9290, 9420,    0,  -16, 7116, 6987, 8396, 8011,
       7882, 7752, 7749, 7875, 8130, 8514, 9027, 9156, 9286, 9289,
       9163, 8780, 8396,   64, 8396, 8139, 8010, 7880, 7877, 8003,
       8130,   64, 8514, 8899, 9028, 9158, 9161, 9035, 8780,   64,
       8130, 7617, 7359, 7229, 7226, 7352, 7735, 8247, 8760, 8889,
       9019, 9022, 8896, 8769, 8514,   64, 8130, 7745, 7487, 7357,
       7354, 7480, 7735,   64, 8247, 8632, 8761, 8891, 8895, 8769,
          0,  -16, 7116, 6987, 9157, 9027, 8769, 8512, 8128, 7873
      };
      short int subbuffer240[100] = {
       7746, 7620, 7623, 7753, 8011, 8396, 8780, 9035, 9162, 9288,
       9284, 9152, 9021, 8762, 8504, 8119, 7735, 7480, 7354, 7355,
       7484, 7611, 7482,   64, 7873, 7747, 7751, 7881, 8139, 8396,
         64, 9035, 9161, 9156, 9024, 8893, 8634, 8376, 8119,    0,
        -16, 7116, 7622, 7993, 7864, 7991, 8120, 7993,    0,  -16,
       7116, 7622, 7991, 7864, 7993, 8120, 8119, 7989, 7731,    0,
        -16, 7116, 7622, 8389, 8260, 8387, 8516, 8389,   64, 7993,
       7864, 7991, 8120,    0,  -16, 7116, 7622, 8389, 8260, 8387,
       8516, 8389,   64, 7991, 7864, 7993, 8120, 8119, 7989, 7731,
          0,  -16, 7116, 7622, 8652, 8523, 8255,   64, 8651, 8255
      };
      short int subbuffer241[100] = {
         64, 8652, 8779, 8255,   64, 7993, 7864, 7991, 8120, 7993,
          0,  -16, 7116, 6987, 7880, 8007, 7878, 7751, 7752, 7882,
       8011, 8396, 8908, 9291, 9417, 9415, 9285, 9156, 8386, 8129,
       8127, 8254, 8510,   64, 8908, 9163, 9289, 9287, 9157, 9028,
       8771,   64, 7993, 7864, 7991, 8120, 7993,    0,  -16, 7116,
       7622, 8780, 8522, 8392, 8391, 8518, 8647, 8520,    0,  -16,
       7116, 7622, 8650, 8523, 8652, 8779, 8778, 8648, 8390,    0,
        -16, 7116, 6605, 9540, 9411, 9538, 9667, 9668, 9541, 9413,
       9156, 8898, 8250, 7992, 7735, 7351, 6968, 6842, 6844, 6974,
       7103, 7360, 8002, 8259, 8517, 8647, 8649, 8523, 8268, 8011
      };
      short int subbuffer242[100] = {
       7881, 7878, 8000, 8125, 8378, 8632, 8887, 9143, 9273, 9274,
         64, 7351, 7096, 6970, 6972, 7102, 7231, 8002,   64, 7878,
       8001, 8126, 8379, 8633, 8888, 9144, 9273,    0,  -16, 7116,
       6987, 8528, 7475,   64, 9168, 8115,   64, 9288, 9159, 9286,
       9415, 9416, 9290, 9163, 8780, 8268, 7883, 7625, 7623, 7749,
       7876, 8768, 9022,   64, 7623, 7877, 8769, 8896, 9022, 9019,
       8889, 8760, 8375, 7863, 7480, 7353, 7227, 7228, 7357, 7484,
       7355,    0,  -16, 7116, 6859, 9936, 6576,    0,  -16, 7116,
       7368, 9296, 8781, 8394, 8135, 7875, 7742, 7738, 7861, 7986,
       8112,   64, 8781, 8393, 8133, 8002, 7869, 7864, 7987, 8112
      };
      short int subbuffer243[100] = {
          0,  -16, 7116, 7239, 8400, 8526, 8651, 8774, 8770, 8637,
       8377, 8118, 7731, 7216,   64, 8400, 8525, 8648, 8643, 8510,
       8379, 8119, 7731,    0,  -16, 7116, 7241, 8524, 8512,   64,
       7881, 9155,   64, 9161, 7875,    0,  -16, 7116, 6605, 7104,
       9408,    0,  -16, 7116, 6605, 8265, 8247,   64, 7104, 9408,
          0,  -16, 7116, 6605, 7107, 9411,   64, 7101, 9405,    0,
        -16, 7116, 7749, 8652, 8389,   64, 8780, 8389,    0,  -16,
       7116, 7113, 8012, 7749,   64, 8140, 7749,   64, 9164, 8901,
         64, 9292, 8901,    0,  -16, 7116, 7368, 8396, 8139, 8009,
       8007, 8133, 8388, 8644, 8901, 9031, 9033, 8907, 8652, 8396
      };
      short int subbuffer244[100] = {
          0,  -16, 7116, 6986, 8268, 7351,   64, 8268, 9143,   64,
       8265, 9015,   64, 7613, 8765,   64, 7095, 7863,   64, 8631,
       9399,    0,  -16, 7116, 6859, 7500, 7479,   64, 7628, 7607,
         64, 7116, 9164, 9158, 9036,   64, 7618, 8642, 9025, 9152,
       9278, 9275, 9145, 9016, 8631, 7095,   64, 8642, 8897, 9024,
       9150, 9147, 9017, 8888, 8631,    0,  -16, 7116, 6859, 7500,
       7479,   64, 7628, 7607,   64, 7116, 8652, 9035, 9162, 9288,
       9286, 9156, 9027, 8642,   64, 8652, 8907, 9034, 9160, 9158,
       9028, 8899, 8642,   64, 7618, 8642, 9025, 9152, 9278, 9275,
       9145, 9016, 8631, 7095,   64, 8642, 8897, 9024, 9150, 9147
      };
      short int subbuffer245[100] = {
       9017, 8888, 8631,    0,  -16, 7116, 7113, 7756, 7735,   64,
       7884, 7863,   64, 7372, 9292, 9286, 9164,   64, 7351, 8247,
          0,  -16, 7116, 6732, 7756, 7750, 7614, 7482, 7352, 7223,
         64, 9036, 9015,   64, 9164, 9143,   64, 7372, 9548,   64,
       6839, 9527,   64, 6839, 6832,   64, 6967, 6832,   64, 9399,
       9520,   64, 9527, 9520,    0,  -16, 7116, 6858, 7500, 7479,
         64, 7628, 7607,   64, 8390, 8382,   64, 7116, 9164, 9158,
       9036,   64, 7618, 8386,   64, 7095, 9143, 9149, 9015,    0,
        -16, 7116, 6352, 8268, 8247,   64, 8396, 8375,   64, 7884,
       8780,   64, 6859, 6986, 6857, 6730, 6731, 6860, 6988, 7115
      };
      short int subbuffer246[100] = {
       7241, 7365, 7491, 7746, 8898, 9155, 9285, 9417, 9547, 9676,
       9804, 9931, 9930, 9801, 9674, 9803,   64, 7746, 7489, 7359,
       7226, 7096, 6967,   64, 7746, 7617, 7487, 7354, 7224, 7095,
       6839, 6712, 6586,   64, 8898, 9153, 9279, 9402, 9528, 9655,
         64, 8898, 9025, 9151, 9274, 9400, 9527, 9783, 9912,10042,
         64, 7863, 8759,    0,  -16, 7116, 6986, 7497, 7372, 7366,
       7497, 7755, 8012, 8524, 8907, 9033, 9030, 8900, 8515, 8131,
         64, 8524, 8779, 8905, 8902, 8772, 8515,   64, 8515, 8770,
       9024, 9150, 9147, 9017, 8888, 8503, 7863, 7608, 7481, 7355,
       7356, 7485, 7612, 7483,   64, 8897, 9022, 9019, 8889, 8760
      };
      short int subbuffer247[100] = {
       8503,    0,  -16, 7116, 6732, 7372, 7351,   64, 7500, 7479,
         64, 9036, 9015,   64, 9164, 9143,   64, 6988, 7884,   64,
       8652, 9548,   64, 9034, 7481,   64, 6967, 7863,   64, 8631,
       9527,    0,  -16, 7116, 6732, 7372, 7351,   64, 7500, 7479,
         64, 9036, 9015,   64, 9164, 9143,   64, 6988, 7884,   64,
       8652, 9548,   64, 9034, 7481,   64, 6967, 7863,   64, 8631,
       9527,   64, 7762, 7763, 7635, 7634, 7760, 8015, 8527, 8784,
       8914,    0,  -16, 7116, 6732, 7372, 7351,   64, 7500, 7479,
         64, 6988, 7884,   64, 7490, 8386, 8643, 8773, 8905, 9035,
       9164, 9292, 9419, 9418, 9289, 9162, 9291,   64, 8386, 8641
      };
      short int subbuffer248[100] = {
       8767, 8890, 9016, 9143,   64, 8386, 8513, 8639, 8762, 8888,
       9015, 9271, 9400, 9530,   64, 6967, 7863,    0,  -16, 7116,
       6604, 7628, 7622, 7486, 7354, 7224, 7095, 6967, 6840, 6841,
       6970, 7097, 6968,   64, 9036, 9015,   64, 9164, 9143,   64,
       7244, 9548,   64, 8631, 9527,    0,  -16, 7116, 6733, 7372,
       7351,   64, 7500, 8250,   64, 7372, 8247,   64, 9164, 8247,
         64, 9164, 9143,   64, 9292, 9271,   64, 6988, 7500,   64,
       9164, 9676,   64, 6967, 7735,   64, 8759, 9655,    0,  -16,
       7116, 6732, 7372, 7351,   64, 7500, 7479,   64, 9036, 9015,
         64, 9164, 9143,   64, 6988, 7884,   64, 8652, 9548,   64
      };
      short int subbuffer249[100] = {
       7490, 9026,   64, 6967, 7863,   64, 8631, 9527,    0,  -16,
       7116, 6859, 8140, 7755, 7497, 7367, 7235, 7232, 7356, 7482,
       7736, 8119, 8375, 8760, 9018, 9148, 9280, 9283, 9159, 9033,
       8779, 8396, 8140,   64, 8140, 7883, 7625, 7495, 7363, 7360,
       7484, 7610, 7864, 8119,   64, 8375, 8632, 8890, 9020, 9152,
       9155, 9031, 8905, 8651, 8396,    0,  -16, 7116, 6732, 7372,
       7351,   64, 7500, 7479,   64, 9036, 9015,   64, 9164, 9143,
         64, 6988, 9548,   64, 6967, 7863,   64, 8631, 9527,    0,
        -16, 7116, 6859, 7500, 7479,   64, 7628, 7607,   64, 7116,
       8652, 9035, 9162, 9288, 9285, 9155, 9026, 8641, 7617,   64
      };
