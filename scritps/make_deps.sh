echo "build 3rd library ..."

export ROOT_DIR=`pwd`
INSTALL_DIR="$ROOT_DIR/pkg"
PKG_DIR="pkg/packages"
RM_DIR=0

if [ ! -d $PKG_DIR ]; then
  mkdir -p $PKG_DIR
fi

if [ $# -ne 1 ]; then
    echo "build_deps.sh <libuv|zlog|spdlog|bdb|pcap>"
    exit -1
fi

if [ $1 == 'libuv' ]; then

	# build libevent
	#
	echo ">>>>>--------> build libuv ..."
	UV_VERSION="1.41.0"
	
	cd $PKG_DIR/
	cd libuv-v$UV_VERSION/

	echo "make..."
	make
	
	echo "install..."
	make install

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

elif [ $1 == 'spdlog' ]; then

	# build spdlog
	#
	echo ">>>>>--------> build spdlog ..."
	SPDLOG_VERSION="1.8.5"

	if [ ! -f $PKG_DIR/spdlog_$SPDLOG_VERSION.tar.gz ]; then
		wget https://github.com/gabime/spdlog/archive/refs/tags/v$SPDLOG_VERSION.tar.gz --output-document=$PKG_DIR/spdlog_$SPDLOG_VERSION.tar.gz
	fi

	cd $PKG_DIR/
	tar -zxvf spdlog_$SPDLOG_VERSION.tar.gz

	cd spdlog-$SPDLOG_VERSION/

	echo "make..."
	mkdir build && cd build
	cmake .. -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR
	make -j

	echo "install..."
	make install
	
	echo "clean..."
	cd ../..
	rm -rf spdlog-$SPDLOG_VERSION/

elif [ $1 == 'bdb' ]; then

	# build berkeleydb
	#
	echo ">>>>>--------> build berkeleydb ..."
	DB_VERSION="18.1.40"

	if [ ! -f $PKG_DIR/db_$DB_VERSION.tar.gz ]; then
		wget https://github.com/junglefire/berkeleydb/archive/refs/tags/$DB_VERSION.tar.gz --output-document=$PKG_DIR/db_$DB_VERSION.tar.gz
	fi

	cd $PKG_DIR/
	tar -zxvf db_$DB_VERSION.tar.gz 

	cd berkeleydb-$DB_VERSION/build_unix

	echo "configure"
	../dist/configure --prefix=$INSTALL_DIR --with-repmgr-ssl=no --disable-replication --enable-stl=no  --enable-tcl=no --enable-localization=no --enable-test=no --enable-java=no --enable-gui=no --enable-mingw=no --enable-dump185=no --enable-compat185=no --disable-partition --disable-queue	

	echo "make"
	make 

	echo "install..."
	make install

	echo "rm doc/ and bin/"
	rm -rf $INSTALL_DIR/docs
	rm -rf $INSTALL_DIR/bin
	
	echo "clean..."
	cd ../..
	rm -rf berkeleydb-$DB_VERSION

elif [ $1 == 'pcap' ]; then

	# build libpcap
	#
	echo ">>>>>--------> build libpcap ..."
	PCAP_VERSION="1.10.1"

	if [ ! -f $PKG_DIR/libpcap-$PCAP_VERSION.tar.gz ]; then
		wget https://www.tcpdump.org/release/libpcap-$PCAP_VERSION.tar.gz --output-document=$PKG_DIR/libpcap-$PCAP_VERSION.tar.gz
	fi

	cd $PKG_DIR/
	tar -zxvf libpcap-$PCAP_VERSION.tar.gz

	cd libpcap-$PCAP_VERSION

	echo "configure"
	./configure --prefix=$INSTALL_DIR	

	echo "make"
	make 

	echo "install..."
	make install

	echo "rm doc/ and bin/"
	rm -rf $INSTALL_DIR/docs
	rm -rf $INSTALL_DIR/bin
	
	echo "clean..."
	cd ../..
	rm -rf libpcap-$PCAP_VERSION

fi
