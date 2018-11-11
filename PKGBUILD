# Maintainer: Matteo Iervasi <matteoiervasi@gmail.com>
# Contributor: Joshua Chapman <john.chy99@gmail.com>
pkgname=sis-bin
pkgver=1.4
pkgrel=1
epoch=
pkgdesc="SIS is the logic synthesis system from UC Berkeley."
arch=('i686' 'x86_64')
url=""
license=('BSD')
groups=()
depends=('readline')
makedepends=()
checkdepends=()
optdepends=()
provides=()
conflicts=()
replaces=()
backup=()
options=('!strip' '!emptydirs')
install=
changelog=
source=("$pkgname-$pkgver.tar.gz"
        "$pkgname-$pkgver.patch")
noextract=()
md5sums=()
validpgpkeys=()

prepare() {
	cd "$pkgname-$pkgver"
	patch -p1 -i "$srcdir/$pkgname-$pkgver.patch"
}

build() {
	cd "$pkgname-$pkgver"
	./configure --prefix=/usr
	make
}

check() {
	cd "$pkgname-$pkgver"
	make -k check
}

package() {
	cd "$pkgname-$pkgver"
	make DESTDIR="$pkgdir/" install
}

