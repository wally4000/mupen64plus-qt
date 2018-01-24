#!/bin/bash

[[ -z $WORKING_DIR ]] && WORKING_DIR=$(pwd)
[[ -z $VERSION ]] && VERSION=$(git log --oneline -n 1 | awk '{print $1}')


case "$1" in

    'setup_qt')
        sudo apt-get update -qq
        sudo apt-get -y install qt5-qmake qtbase5-dev libqt5sql5-sqlite zlib1g-dev
    ;;

    'get_quazip')
        wget http://downloads.sourceforge.net/quazip/quazip-0.7.3.tar.gz
        tar -xvzf quazip-0.7.3.tar.gz > /dev/null
        mv quazip-0.7.3/quazip quazip5
    ;;

    'build')
        ./build-scripts/revision.sh
        qmake -qt=qt5 CONFIG+=linux_quazip_static
        make
    ;;

    'package')
        mkdir -p "build/$TRAVIS_BRANCH"

        mv resources/README.txt .
        tar -cvzpf "build/$TRAVIS_BRANCH/mupen64plus-qt_linux_$VERSION.tar.gz" mupen64plus-qt README.txt
    ;;

esac
