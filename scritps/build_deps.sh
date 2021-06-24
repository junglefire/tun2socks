echo "build 3rd library ..."

export ROOT_DIR=`pwd`
INSTALL_DIR="$ROOT_DIR/pkg"
PKG_DIR="pkg/packages"

if [ ! -d $PKG_DIR ]; then
  mkdir -p $PKG_DIR
fi

if [ $# -ne 1 ]; then
    echo "build_deps.sh <libuv|zlog>"
    exit -1
fi

if [ $1 == 'libuv' ]; then

	# build libevent
	#
	echo ">>>>>--------> build libuv ..."
	UV_VERSION="1.41.0"
	if [ ! -f $PKG_DIR/libuv-v$UV_VERSION.tar.gz ]; then 
		wget https://dist.libuv.org/dist/v$UV_VERSION/libuv-v$UV_VERSION.tar.gz --output-document=$PKG_DIR/libuv-v$UV_VERSION.tar.gz
	fi
	
	cd $PKG_DIR/
	tar -zxvf libuv-v$UV_VERSION.tar.gz

	cd libuv-v$UV_VERSION/

	echo "make..."
	sh autogen.sh
	./configure --prefix=$INSTALL_DIR
	make
	
	echo "install..."
	make install

	echo "clean..."
	cd ../
	rm -rf libuv-v$UV_VERSION/

elif [ $1 == 'zlog' ]; then

	# build zlog
	#
	echo ">>>>>--------> build zlog ..."
	ZLOG_VERSION="1.2.15"

	if [ ! -f $PKG_DIR/zlog_$ZLOG_VERSION.tar.gz ]; then
		wget https://github.com/junglefire/zlog/archive/$ZLOG_VERSION.tar.gz --output-document=$PKG_DIR/zlog_$ZLOG_VERSION.tar.gz
	fi

	cd $PKG_DIR/
	tar -zxvf zlog_$ZLOG_VERSION.tar.gz

	cd zlog-$ZLOG_VERSION/

	echo "make..."
	make

	echo "install..."
	make PREFIX=$INSTALL_DIR/ install
	rm $INSTALL_DIR/lib/libzlog*dylib
	
	echo "clean..."
	cd ..
	rm -rf zlog-$ZLOG_VERSION/

fi
