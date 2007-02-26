#!/bin/sh
# Run this to generate all the initial makefiles, etc.
#
# Use --build-w32 to prepare the cross compiling build for Windoze
#

PGM=GPA
autoconf_vers=2.52
automake_vers=1.6
aclocal_vers=1.6
libtool_vers=1.4

DIE=no
FORCE=
if test "$1" == "--force"; then
  FORCE=" --force"
  shift
fi


if test "$1" = "--build-w32"; then
    tmp=`dirname $0`
    tsdir=`cd "$tmp"; pwd`
    shift
    if [ ! -f $tsdir/config.guess ]; then
        echo "$tsdir/config.guess not found" >&2
        exit 1
    fi
    build=`$tsdir/config.guess`

    [ -z "$w32root" ] && w32root="$HOME/w32root"
    export w32root
    echo "Using $w32root as standard install directory" >&2
    
    # See whether we have the Debian cross compiler package or the
    # old mingw32/cpd system
    if i586-mingw32msvc-gcc --version >/dev/null 2>&1 ; then
       host=i586-mingw32msvc
       crossbindir=/usr/$host/bin
       conf_CC="CC=${host}-gcc"
    else
       host=i386--mingw32
       if ! mingw32 --version >/dev/null; then
          echo "We need at least version 0.3 of MingW32/CPD" >&2
          exit 1
       fi
       crossbindir=`mingw32 --install-dir`/bin
       # Old autoconf version required us to setup the environment
       # with the proper tool names.
       CC=`mingw32 --get-path gcc`
       CPP=`mingw32 --get-path cpp`
       AR=`mingw32 --get-path ar`
       RANLIB=`mingw32 --get-path ranlib`
       export CC CPP AR RANLIB 
       conf_CC=""
    fi
   
    if [ -f "$tsdir/config.log" ]; then
        if ! head $tsdir/config.log | grep "$host" >/dev/null; then
            echo "Pease run a 'make distclean' first" >&2
            exit 1
        fi
    fi

    $tsdir/configure ${conf_CC} --build=${build} --host=${host} \
            --prefix=${w32root} \
            --with-zlib=${w32root} \
            --with-gpg-error-prefix=${w32root} \
	    --with-gpgme-prefix=${w32root} \
            --with-lib-prefix=${w32root} \
            --with-libiconv-prefix=${w32root} \
            PKG_CONFIG_LIBDIR="$w32root/lib/pkgconfig"
    exit $?
fi



if (autoconf --version) < /dev/null > /dev/null 2>&1 ; then
    if (autoconf --version | awk 'NR==1 { if( $3 >= '$autoconf_vers') \
			       exit 1; exit 0; }');
    then
       echo "**Error**: "\`autoconf\'" is too old."
       echo '           (version ' $autoconf_vers ' or newer is required)'
       DIE="yes"
    fi
else
    echo
    echo "**Error**: You must have "\`autoconf\'" installed to compile $PGM."
    echo '           (version ' $autoconf_vers ' or newer is required)'
    DIE="yes"
fi

if (automake --version) < /dev/null > /dev/null 2>&1 ; then
  if (automake --version | awk 'NR==1 { if( $4 >= '$automake_vers') \
			     exit 1; exit 0; }');
     then
     echo "**Error**: "\`automake\'" is too old."
     echo '           (version ' $automake_vers ' or newer is required)'
     DIE="yes"
  fi
  if (aclocal --version) < /dev/null > /dev/null 2>&1; then
    if (aclocal --version | awk 'NR==1 { if( $4 >= '$aclocal_vers' ) \
						exit 1; exit 0; }' );
    then
      echo "**Error**: "\`aclocal\'" is too old."
      echo '           (version ' $aclocal_vers ' or newer is required)'
      DIE="yes"
    fi
  else
    echo
    echo "**Error**: Missing "\`aclocal\'".  The version of "\`automake\'
    echo "           installed doesn't appear recent enough."
    DIE="yes"
  fi
else
    echo
    echo "**Error**: You must have "\`automake\'" installed to compile $PGM."
    echo '           (version ' $automake_vers ' or newer is required)'
    DIE="yes"
fi


if (gettext --version </dev/null 2>/dev/null | awk 'NR==1 { split($4,A,"."); \
    X=10000*A[1]+100*A[2]+A[3]; echo X; if( X >= 1201 ) exit 1; exit 0}')
    then
    echo "**Error**: You must have "\`gettext\'" installed to compile $PGM."
    echo '           (version 0.12.1 or newer is required; get'
    echo '            ftp://alpha.gnu.org/gnu/gettext-0.12.1.tar.gz'
    echo '            or install the latest Debian package)'
    DIE="yes"
fi


#if (libtool --version) < /dev/null > /dev/null 2>&1 ; then
#    if (libtool --version | awk 'NR==1 { if( $4 >= '$libtool_vers') \
#			       exit 1; exit 0; }');
#    then
#       echo "**Error**: "\`libtool\'" is too old."
#       echo '           (version ' $libtool_vers ' or newer is required)'
#       DIE="yes"
#    fi
#else
#    echo
#    echo "**Error**: You must have "\`libtool\'" installed to compile $PGM."
#    echo '           (version ' $libtool_vers ' or newer is required)'
#    DIE="yes"
#fi


if test "$DIE" = "yes"; then
    exit 1
fi

echo "Running autopoint"
autopoint
echo "Running aclocal..."
aclocal -I m4
echo "Running autoheader..."
autoheader
echo "Running automake --add-missing --gnu ..."
automake --add-missing --gnu
echo "Running autoconf${FORCE} ..."
autoconf${FORCE}

echo "You may now run \"./configure --enable-maintainer-mode\" and then \"make\"."
