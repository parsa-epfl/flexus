export BOOST="boost_1_70_0"
export BOOST_VERSION="1.70.0"
# wget https://boostorg.jfrog.io/artifactory/main/release/${BOOST_VERSION}/source/${BOOST}.tar.gz -O /tmp/${BOOST}.tar.gz
# tar -xf /tmp/${BOOST}.tar.gz
cd ./${BOOST}/
./bootstrap.sh --prefix=/usr/local
./b2 -j8
sudo ./b2 install
