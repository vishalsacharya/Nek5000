function make_3rd_party() {
  if [ $NOCOMPILE -ne 0 ]; then
    return 0
  fi
  echo "build 3rd-party software ..."

  CFLAGS_=$CFLAGS
  if ! echo $CFLAGS | grep -q '\-O' ; then 
     CFLAGS+=" -O2"
  fi 
  FFLAGS_=$FFLAGS
  if ! echo $FFLAGS | grep -q '\-O' ; then 
     FFLAGS+=" -O2"
  fi 

  cd $SOURCE_ROOT_GSLIB
  ./install
  cd $CASEDIR 

  if [ $PARRSB -ne 0 ]; then
     cd $SOURCE_ROOT_PARRSB
     ./install
  fi
  cd $CASEDIR

  if [ $PARMETIS -ne 0 ]; then
     cd $SOURCE_ROOT_PARMETIS
     ./install
  fi
  cd $CASEDIR

  if [ $CVODE -ne 0 ]; then
     cd $SOURCE_ROOT_CVODE
     ./install
  fi
  cd $CASEDIR

  if [ $HYPRE -ne 0 ]; then
     cd $SOURCE_ROOT_HYPRE
     ./install
  fi
  cd $CASEDIR

  CFLAGS=$CFLAGS_
  export CFLAGS
  FFLAGS=$FFLAGS_
  export FFLAGS
}

function clean_3rd_party() {
  echo "cleaning 3rd-party software ..."
  cd $SOURCE_ROOT_GSLIB; ./install clean 2>/dev/null
  cd $CASEDIR
  cd $SOURCE_ROOT_PARRSB; ./install clean 2>/dev/null
  cd $CASEDIR
  cd $SOURCE_ROOT_PARMETIS; ./install clean 2>/dev/null
  cd $CASEDIR
  cd $SOURCE_ROOT_CVODE; ./install clean 2>/dev/null
  cd $CASEDIR
  cd $SOURCE_ROOT_HYPRE; ./install clean 2>/dev/null
  cd $CASEDIR
}

# This include file is used by the makenek script
# to automatically create a makefile for Nek5000 
# ------------------------------------------------

echo "makenek - automatic build tool for Nek5000"

if [ "$PPLIST" == "?" ]; then
  echo "available pre-processor symbols:" 
  echo "  NOMPIIO     deactivate MPI-IO support"
  echo "  BGQ         use BGQ optimized mxm"
  echo "  XSMM        use libxsmm for mxm"
  echo "  CVODE       compile with CVODE support for scalars"
  echo "  HYPRE       compile with HYPRE support"
  echo "  VENDOR_BLAS use VENDOR BLAS/LAPACK"
  exit 1
fi

# first do some checks ...
if [ "$1" == "-h" ]; then
  echo ""
  echo "usage: makenek [.usr filename | clean] < -nocompile >"
  echo ""
  exit 1
fi

CASEDIR=`pwd`
CASENAME=$1
if [ $# -eq 0 ]; then
   NCASEFILES=`find *.usr 2>/dev/null| wc -l`
   if [ $NCASEFILES == "0" ]; then
     echo "FATAL ERROR: Cannot find .usr file!"
     exit 1
   elif [ $NCASEFILES != "1" ]; then
     echo "FATAL ERROR: Found multiple .usr files!"
     echo "Please use makenek [.usr filename]"
     exit 1
   else
     CASENAME=`find *.usr | sed 's/\.usr$//g'`
   fi
fi

if [ "$mver" != "18.0.0" ]; then
  echo "FATAL ERROR: Your makenek is incompatible!"
  echo "Please upgrade to the version shipped with your Nek source."
  exit 1
fi

SOURCE_ROOT_GSLIB="$NEK_SOURCE_ROOT/3rd_party/gslib"
export SOURCE_ROOT_GSLIB

SOURCE_ROOT_PARRSB="$NEK_SOURCE_ROOT/3rd_party/parRSB"
export SOURCE_ROOT_PARRSB

SOURCE_ROOT_PARMETIS="$NEK_SOURCE_ROOT/3rd_party/parMETIS"
export SOURCE_ROOT_PARMETIS

SOURCE_ROOT_CVODE="$NEK_SOURCE_ROOT/3rd_party/cvode"
export SOURCE_ROOT_CVODE

SOURCE_ROOT_HYPRE="$NEK_SOURCE_ROOT/3rd_party/hypre"
export SOURCE_ROOT_HYPRE

if [[ $1 = "clean" ]]; then
  while true; do
    read -p "do you want to clean all 3rd party software? [N]" yn
    case $yn in
      [Yy]* )
        clean_3rd_party; 
        make clean;
        break ;;
      * ) 
        make clean;
        break ;;
    esac
  done
  rm -rf compiler.out
  rm -rf makefile
  rm -rf .state
  exit 0
fi

NOCOMPILE=0
if [[ "$2" == "-nocompile" ]]; then
  NOCOMPILE=1
fi 

if [ ! -f ${CASENAME}'.usr' ]; then
   echo "FATAL ERROR: Cannot find $1.usr!"
   exit 1
fi

if [ -f SIZEu ]; then
   if [ -f SIZE ]; then
      echo "FATAL ERROR: found SIZEu and SIZE!"
      exit 1
   fi
   NEK_WARN="WARNING: SIZEu will be deprecated in a future release!"
   mv -v SIZEu SIZE
fi

if [ ! -f SIZE ]; then
   echo "FATAL ERROR: Cannot find SIZE!"
   exit 1
fi

if [ ! -f $NEK_SOURCE_ROOT/core/makefile.template ]; then
  echo "FATAL ERROR: Cannot find $NEK_SOURCE_ROOT/core/makefile.template!"
  exit 1
fi

# test if FC compiler exists
if ! which `echo $FC | awk '{print $1}'` 1>/dev/null ; then
  echo "FATAL ERROR: Cannot find `echo $FC | awk '{print $1}'`!"
  exit 1
fi


# test if C compiler exists
if ! which `echo $CC | awk '{print $1}'` 1>/dev/null ; then 
  echo "FATAL ERROR: Cannot find `echo $CC | awk '{print $1}'`!"
  exit 1
fi

# trying to figure which compiler the wrapper is using 
FCok=0

FCcomp_=`$FC -showme 2>/dev/null | head -1 2>/dev/null 1>.tmp || true`
FCcomp=`cat .tmp | awk '{print $1}' | awk -F/ '{print $NF}' || true`
if [ -f "`which $FCcomp 2>/dev/null`" ]; then
  FCok=1
fi

if [ $FCok -eq 0 ]; then
  FCcomp_=`$FC -show 2>/dev/null | head -1 2>/dev/null 1>.tmp || true`
  FCcomp=`cat .tmp | awk '{print $1}' | awk -F/ '{print $NF}' || true`
  if [ -f "`which $FCcomp 2>/dev/null`" ]; then
    FCok=1
  fi
fi

if [ $FCok -eq 0 ]; then
  FCcomp_=`$FC -craype-verbose 2>/dev/null 1>.tmp || true`
  FCcomp=`cat .tmp | awk '{print $1}' | awk -F/ '{print $NF}' || true`
  if [ -f "`which $FCcomp 2>/dev/null`" ]; then
    FCok=1
  fi
fi

if [ $FCok -eq 0 ]; then
  FCcomp=`echo $FC | awk '{print $1}'`
  if [ -f "`which $FCcomp 2>/dev/null`" ]; then
    FCok=1
  fi
fi

\rm -f .tmp
if [ $FCok -eq 0 ]; then
  FCcomp="unknown"
fi

# assign FC compiler specific flags
case $FCcomp in
  *pgf*)        FFLAGS+=" -r8 -Mpreprocess"
               ;;
  *gfortran*)   FFLAGS+=" -std=legacy -fdefault-real-8 -fdefault-double-8 -cpp"
               ;;
  *ftn*)       FFLAGS+=" -sreal64 -eZ"
               ;;
  *armflang*)   FFLAGS+=" -r8 -cpp -std=legacy"
               ;;
  *ifort*)      FFLAGS+=" -r8 -fpconstant -fpp"
               ;;
  *xlf*)        FFLAGS+=" -qfixed -qrealsize=8 -qdpc=e -qsuffix=cpp=f -qsuppress=cmpmsg"
                PPPO="-WF,"
               ;;
  *)  echo "ERROR: Cannot find a supported compiler!"
      echo ""
      NOCOMPILE=1
      read;;
esac

# initial clean-up
rm -f $CASENAME.f nek5000 2>/dev/null
rm -f ./obj/subuser.o 2>/dev/null

# Check if the compiler adds an underscore to external functions
UNDERSCORE=0
cat > test_underscore.f << _ACEOF
      subroutine underscore_test
        call test
      end
_ACEOF
$FCcomp -c test_underscore.f 2>&1 >/dev/null 
if nm test_underscore.o | grep -q test_ ; then
  UNDERSCORE=1
fi
\rm test_underscore.* 2>/dev/null

# Check if ld option is available
cat > _test.f << _ACEOF
      program test
      end
_ACEOF
$FCcomp -Wl,--allow-multiple-definition -o _test _test.f >/dev/null 2>&1 || true
\rm _test.f 2>/dev/null
if [ -f _test ]; then
  LDFLAGS+=" -Wl,--allow-multiple-definition"
  \rm _test 2>/dev/null
fi

# set preprocessor symbols
if [ $MPI -eq 0 ]; then
  PPLIST="${PPLIST} NOMPIIO"
else
  MPI=1
  PPLIST="${PPLIST} MPI"
fi

if [ $UNDERSCORE -ne 0 ]; then
  PPLIST="${PPLIST} UNDERSCORE"
fi 

if [ $PROFILING -ne 0 ]; then
  if [ $MPI -ne 0 ]; then
    PPLIST="${PPLIST} TIMER"
  else
    PPLIST="${PPLIST} TIMER"
  fi
fi

PPLIST="${PPLIST}"

# add user defined preprocessor symbols
CMT=0
MPIIO=1
DPROCMAP=0
PARRSB=0
PARMETIS=0
CVODE=0
HYPRE=0
MXM_USER="mxm_std.o"
BLAS="blas.o dsygv.o"

for i in $PPLIST
do
   if [ "$i" == "CMTNEK" ]; then
     CMT=1
   fi
   if [ "$i" == "NOMPIIO" ]; then
     MPIIO=0
   fi
   if [ "$i" == "DPROCMAP" ]; then
     DPROCMAP=1
   fi
   if [ "$i" == "PARRSB" ]; then
     PARRSB=1
     PARRSB_OPT=" DESTDIR=.. UNDERSCORE=$UNDERSCORE MPI=$MPI" 
     PARRSB_OPT+=" GSLIBPATH=$SOURCE_ROOT_GSLIB"
     USR_LFLAGS+=" -L$SOURCE_ROOT_PARRSB/lib -lparRSB"
     PARRSB_INC="$SOURCE_ROOT_PARRSB/include"
   fi
   if [ "$i" == "PARMETIS" ]; then
     PARMETIS=1
     USR_LFLAGS+=" -L$SOURCE_ROOT_PARMETIS/lib -lparmetis -lmetis"
     METIS_INC="$SOURCE_ROOT_PARMETIS/include"
   fi
   if [ "$i" == "CVODE" ]; then 
     CVODE=1
     if [ $MPI -eq 1 ]; then
       USR_LFLAGS+=" -L$SOURCE_ROOT_CVODE/lib -lsundials_fcvode -lsundials_cvode"
       USR_LFLAGS+=" -lsundials_fnvecparallel -lsundials_nvecparallel" 
     else
       USR_LFLAGS+=" -L$SOURCE_ROOT_CVODE/lib -lsundials_fcvode -lsundials_cvode"
       USR_LFLAGS+=" -lsundials_fnvecserial -lsundials_nvecserial" 
     fi
   fi
   if [ "$i" == "HYPRE" ]; then
     HYPRE=1
     USR_LFLAGS+=" -L$SOURCE_ROOT_HYPRE/lib -lHYPRE"
     HYPRE_INC="$SOURCE_ROOT_HYPRE/include"
   fi
   if [ "$i" == "BGQ" ]; then
     MXM_USER+=" mxm_bgq.o" 
   fi
   if [ "$i" == "VENDOR_BLAS" ]; then
     BLAS=" "
   fi
done

# gslib build options
GSLIB_PREFIX="gslib_"
GSLIB_FPREFIX="fgslib_"
GSLIB_OPT+=" DESTDIR=.."
GSLIB_OPT+=" MPI=$MPI MPIIO=$MPIIO"
GSLIB_OPT+=" ADDUS=$UNDERSCORE USREXIT=0 BLAS=2"
GSLIB_OPT+=" PREFIX=$GSLIB_PREFIX FPREFIX=$GSLIB_FPREFIX"
GSLIB_IFLAGS="-DPREFIX=$GSLIB_PREFIX -DFPREFIX=$GSLIB_FPREFIX -DGLOBAL_LONG_LONG"
GSLIB_IFLAGS+=" -I$SOURCE_ROOT_GSLIB/include"
USR_LFLAGS+=" -L$SOURCE_ROOT_GSLIB/lib -lgs"

# tweak makefile template 
echo "generating makefile ..."
rm -rf makefile 2>/dev/null

sed -e "s:^FC[ ]*=.*:FC=$FC:" \
-e "s:^CC[ ]*=.*:CC=$CC:" \
-e "s:^FFLAGS[ ]*=.*:FFLAGS=$FFLAGS:" \
-e "s:^CFLAGS[ ]*=.*:CFLAGS=$CFLAGS:" \
-e "s:^LDFLAGS[ ]*=.*:LDFLAGS=$LDFLAGS:" \
-e "s/^PPPO[ ]*=.*/PPPO=$PPPO/" \
-e "s/^PPS[ ]*=.*/PPS=$PPLIST/" \
-e "s:^MXM[ ]*=.*:MXM=$MXM_USER:" \
-e "s:^BLAS[ ]*=.*:BLAS=$BLAS:" \
-e "s/^MPI[ ]*=.*/MPI=$MPI/" \
-e "s/^CVODE[ ]*=.*/CVODE=$CVODE/" \
-e "s/^CMT[ ]*=.*/CMT=$CMT/" \
-e "s:^GSLIB_IFLAGS[ ]*=.*:GSLIB_IFLAGS\:=${GSLIB_IFLAGS}:" \
-e "s/^DPROCMAP[ ]*=.*/DPROCMAP=${DPROCMAP}/" \
-e "s/^VISIT[ ]*=.*/VISIT=${VISIT}/" \
-e "s:^USR[ ]*=.*:USR=$USR:" \
-e "s:^USR_LFLAGS[ ]*=.*:USR_LFLAGS=$USR_LFLAGS:" \
-e "s:^S[ ]*=.*:S=${NEK_SOURCE_ROOT}/core:" \
-e "s:^SCMT[ ]*=.*:SCMT=${NEK_SOURCE_ROOT}/core/cmt:" \
$NEK_SOURCE_ROOT/core/makefile.template >.makefile

if [ $VISIT -ne 0 ]; then
  echo ""
  echo "Activate VisIt Sim Mode"

  if [ -z "$VISIT_INC" ]; then
    VISIT_INC=${VISIT_INSTALL}/libsim/V2/include
  fi
  echo "  VISIT_INC = ($VISIT_INC)"
  if [ -z "$VISIT_LIB" ]; then
    VISIT_LIB=${VISIT_INSTALL}/libsim/V2/lib
  fi
  echo "  VISIT_LIB = ($VISIT_LIB)"
  if [ -n "$VISIT_STOP" ]; then
    VISIT_STOP="VISIT_STOP"
    echo "  VISIT_STOP is defined. Nek5000 will wait for VisIt to Connect."
  fi

  sed -e "s/^VISIT[ ]*=.*/VISIT:=${VISIT}/" \
      -e "s:^VISIT_IFLAGS[ ]*=.*:VISIT_IFLAGS\:=-I${VISIT_INC}:" \
      -e "s:^VISIT_LFLAGS[ ]*=.*:VISIT_LFLAGS\:=-L${VISIT_LIB} -lsimV2 -lsimV2f -ldl -lstdc++:" \
      -e "s/^PPS[ ]*=.*/PPS=${PPLIST} VISIT ${VISIT_STOP}/" \
      .makefile > .makefile.tmp

  mv .makefile.tmp .makefile
  echo ""
fi

if [ $HYPRE -ne 0 ]; then
  sed -e "s:^HYPRE_IFLAGS[ ]*=.*:HYPRE_IFLAGS\:=-I${HYPRE_INC}:" \
      -e "s:^USR_LFLAGS[ ]*=.*:USR_LFLAGS=$USR_LFLAGS:" \
      .makefile > .makefile.tmp

  mv .makefile.tmp .makefile
fi

if [ $PARMETIS -ne 0 ]; then
  sed -e "s:^PARRSB_IFLAGS[ ]*=.*:PARRSB_IFLAGS\:=-I${METIS_INC}:" \
      -e "s:^USR_LFLAGS[ ]*=.*:USR_LFLAGS=$USR_LFLAGS:" \
      .makefile > .makefile.tmp

  mv .makefile.tmp .makefile
fi

if [ $PARRSB -ne 0 ]; then
  sed -e "s:^PARRSB_IFLAGS[ ]*=.*:PARRSB_IFLAGS\:=-I${PARRSB_INC}:" \
      -e "s:^USR_LFLAGS[ ]*=.*:USR_LFLAGS=$USR_LFLAGS:" \
      .makefile > .makefile.tmp

  mv .makefile.tmp .makefile
fi


if [ "$USR" != "" ]; then
  echo "###########################################################" >> makefile
  echo "include makefile_usr.inc" >> .makefile
fi

if [ -f .makefile ]; then
  sed -e "1i\\
### makefile automatically created by makenek `date +"%m/%d/%Y %T"` ###" \
-e "s:^CASEDIR[ ]*=.*:CASEDIR\:=${CASEDIR}:" \
-e "s:^CASENAME[ ]*=.*:CASENAME\:=${CASENAME}:" .makefile > makefile 
else
  echo "ERROR: Nek Makefile could not be created!"
  exit 1 
fi
\rm .makefile 2>/dev/null

# tweak SIZE file
if ! cat SIZE | grep -qi 'lelr' ; then
   echo >>SIZE
   echo 'c automatically added by makenek' >>SIZE
   echo '      integer lelr' >>SIZE
   echo '      parameter (lelr=lelt) ! max number of local elements per restart file' >>SIZE
fi
if ! cat SIZE | grep -qi 'ldimt_proj' ; then
     echo >>SIZE
     echo 'c automatically added by makenek' >>SIZE
     echo '      integer ldimt_proj' >>SIZE
     echo '      parameter(ldimt_proj=1) ! max auxiliary fields residual projection ' >>SIZE
fi

# tweak leagacy SIZE file
if ! cat SIZE | grep -q 'SIZE.inc' ; then
  NEK_WARN="WARNING: Deprecated SIZE will be removed in a future release!"
  if ! cat SIZE | grep -qi 'optlevel' ; then
     echo >>SIZE
     echo 'c automatically added by makenek' >>SIZE
     echo '      integer optlevel,loglevel' >>SIZE
     echo '      common /lolevels/ optlevel,loglevel' >>SIZE
  fi
  if ! cat SIZE | grep -qi 'lxo' ; then
     echo >>SIZE
     echo 'c automatically added by makenek' >>SIZE
     echo '      parameter(lxo   = lx1) ! max output grid size (lxo>=lx1)' >>SIZE
  fi
  if ! cat SIZE | grep -qi 'ax1' ; then
     echo >>SIZE
     echo 'c automatically added by makenek' >>SIZE
     echo '      integer ax1,ay1,az1,ax2,ay2,az2' >> SIZE
     echo '      parameter (ax1=lx1,ay1=ly1,az1=lz1,ax2=lx2,ay2=ly2,az2=lz2) ! running averages' >> SIZE
  fi
  if ! cat SIZE | grep -qi 'lys=lxs' ; then
     cat SIZE | grep -iv lxs > SIZE.x; mv SIZE.x SIZE  # Clean existing SIZE file of old version
     echo >>SIZE
     echo 'c automatically added by makenek' >>SIZE
     echo '      parameter (lxs=1,lys=lxs,lzs=(lxs-1)*(ldim-2)+1) !New Pressure Preconditioner' >> SIZE

  fi
  if ! cat SIZE | grep -qi 'lcvx1' ; then
     echo >>SIZE
     echo 'c automatically added by makenek' >>SIZE
     echo '      integer lcvx1,lcvy1,lcvz1,lcvelt' >> SIZE
     echo '      parameter (lcvx1=1,lcvy1=1,lcvz1=1,lcvelt=1) ! cvode arrays' >> SIZE
  fi
  if ! cat SIZE | grep -qi 'lfdm' ; then
     echo >>SIZE
     echo 'c automatically added by makenek' >>SIZE
     echo '      parameter (lfdm=0)  ! == 1 for fast diagonalization method' >> SIZE
  fi
  if ! cat SIZE | grep -qi 'nsessmax' ; then
     echo >>SIZE
     echo 'c automatically added by makenek' >>SIZE
     echo '      parameter (nsessmax=1)  ! max sessions to NEKNEK' >> SIZE
  fi
  if ! cat SIZE | grep -qi 'nmaxl_nn' ; then
     echo >>SIZE
     echo 'c automatically added by makenek' >>SIZE
     echo '      parameter (nmaxl_nn=' >> SIZE
     echo '     $          min(1+(nsessmax-1)*2*ldim*lxz*lelt,2*ldim*lxz*lelt))' >>SIZE
  fi
  if ! cat SIZE | grep -qi 'nfldmax_nn' ; then
     echo >>SIZE
     echo 'c automatically added by makenek' >>SIZE
     echo '      parameter (nfldmax_nn=' >> SIZE
     echo '     $          min(1+(nsessmax-1)*(ldim+1+ldimt),ldim+1+ldimt))' >>SIZE
  fi
  if ! cat SIZE | grep -qi 'nio' ; then
     echo >>SIZE
     echo 'c automatically added by makenek' >>SIZE
     echo '      common/IOFLAG/nio  ! for logfile verbosity control' >> SIZE
  fi

  if ! cat SIZE | grep -qi 'toteq' ; then
  if [ $CMT -ne 0 ]; then
      echo >>SIZE
      echo 'c automatically added by makenek' >>SIZE
      echo '      integer toteq' >> SIZE
      echo '      parameter(toteq = 5  ) ! Number of conserved variables '  >>SIZE
  fi
  fi
fi

export FC
export CC
export NEK_WARN 
export MPI
export MPIO
export FFLAGS
export CFLAGS
export UNDERSCORE
export GSLIB_OPT
export PARRSB_OPT

if [ $NOCOMPILE -ne 0 ]; then
  exit 0
fi

declare -a varlist=(
"NEK_SOURCE_ROOT"
"FC"
"FFLAGS"
"CC"
"CFLAGS"
"PPLIST"
)
if [ -f .state ]; then
  for i in "${varlist[@]}"
  do
    if ! cat .state | grep -q "$i=${!i}" ; then
      echo "configuration change detected, forcing full rebuild"
      rm -rf .state
      while true; do
        read -p "do you want to rebuild all 3rd party software? [N]" yn
        case $yn in
          [Yy]* )
            clean_3rd_party;
            break ;;
          * )
           break ;;
        esac
      done  
      make clean;
      break
    fi
  done ;
fi

rm -rf .state
for i in "${varlist[@]}"
do
  echo "$i=${!i}" >> .state
done ;
