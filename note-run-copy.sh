#!/bin/bash
#note-run-copy.sh

srcdir=tinypy-googlecode
dstdir=tinypy-git

if [ ! -f note-verfile ]; then
  vervar=1
else
  vervar=$((`cat note-verfile` + 1))
fi

echo
echo " **** Getting version $vervar **** "
echo

echo pwd : 
pwd

(cd $srcdir && svn update -r $vervar )
  rc=$?; if [ $rc -ne 0 ]; then echo Error $rc ; exit $rc ; fi
echo >> note-versions
echo $vervar > note-verfile

(cd $srcdir && (svn log -l 1 -v >> ../note-versions))
  rc=$?; if [ $rc -ne 0 ]; then echo Error $rc ; exit $rc ; fi

echo
echo " **** Git adding version $vervar **** "
echo

echo pwd : 
pwd
(cd $dstdir && cp -a ../$srcdir/* . && cp -a ../note-versions . )
  rc=$?; if [ $rc -ne 0 ]; then echo Error $rc ; exit $rc ; fi
(cd $dstdir && git add -A  )
  rc=$?; if [ $rc -ne 0 ]; then echo Error $rc ; exit $rc ; fi
(cd $dstdir && git commit -m "svn-$vervar" )
  rc=$?; if [ $rc -ne 0 ]; then echo Error $rc ; exit $rc ; fi

echo
echo " **** Git OK version $vervar **** "
echo


