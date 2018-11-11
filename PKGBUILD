# Maintainer: Matteo Iervasi
# Contributor: Joshua Chapman
pkgname=sis-bin
pkgver=1.4
pkgrel=1
pkgdesc="SIS is the logic synthesis system from UC Berkeley."
arch=('i686' 'x86_64')
url=""
license=('BSD')
groups=('')
depends=('readline')
options=('!strip' '!emptydirs')
source_i686=("PUT_FULL_URL_FOR_DOWNLOADING_i386_DEB_PACKAGE_HERE")
source_x86_64=("PUT_FULL_URL_FOR_DOWNLOADING_amd64_DEB_PACKAGE_HERE")
sha512sums_i686=('PUT_SHA512SUM_OF_i386_DEB_PACKAGE_HERE')
sha512sums_x86_64=('93a444eaf6e52f98da6803d9847e4c36f561843067923660ebfdae92fbf02719ab731cd5e619a221d5b0771b20fee5782326e7797f368279b62105c435899eef')

package(){
    ./configure
    make
    make install
}
