# medical-image-analysis

## Author: Andreas Hadjiprocpis (andreashad2@gmail.com)
## Institute of Neurology

My work at the Institute of Neurology (UCL, Queen Square, London).

It is a toolkit of reading and analysing brain MRI scans.

The analysis consists of segmenting multiple sclerosis lesions
and brain gray and white matter and CSF (the fluid in the brain).

There are also a lot of auxiliary utilities (e.g. clustering)
all written in C for efficiency.

# download
All files in this repo are for browsing only. If you want to install
then you need to download only the tarball:

``https://raw.githubusercontent.com/hadjiprocopis/medical-image-analysis/master/iontoolkit-2.0.0.tar.gz``

# install
Extract the files from the tar file using

```tar xvzf iontoolkit-2.0.0.tar.gz```

change dir to application:

```cd iontoolkit-2.0.0```

configure (note that below we configure for the install dir to be at
```$HOME/usr```, which means a folder in you home dir so that we avoid
requesting root privileges for installing files. This is the best
option in my opinion. Then all binaries will be placed in ```$HOME/usr/bin```
and include files and libraries in ```$HOME/usr/include``` and ```$HOME/usr/lib```
respectively. Alternatively, if you do have root privileges do not
specify ```--prefix``` in the command below. And all files will be installed
in system default locations. The other two options are required and please
do not forget them. Later GNU compiler versions are so pedantic they will
fail on very stupid warnings.):

```./configure --prefix=$HOME/usr --disable-werror --disable-debug```

compile:

```make clean; make all```

install in specified installation dir:

```make install```

In order to execute a command:

```$HOME/usr/bin/UNCtest -o output.unc```

This will produce a test UNC file as specified.

If you have ```$HOME/usr/bin``` in your path
(e.g. ```EXPORT PATH="$PATH:$HOME/usr/bin"```)
then you do not need to prefix each command with the
path.

This suite of programs and libraries has an important
dependency : FFTW3. This is a Fast Fourier Transform library by the good people
Matteo Frigo and Steven G. Johnson, see "The Design and Implementation of FFTW3," Proceedings of the IEEE 93 (2), 216–231 (2005). Invited paper, Special Issue on Program Generation, Optimization, and Platform Adaptation.
(see http://www.fftw.org/ for downloading). Thanks to them!

## Author: Andreas Hadjiprocpis (andreashad2@gmail.com)
## Institute of Neurology

