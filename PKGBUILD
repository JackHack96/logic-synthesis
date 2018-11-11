# Maintainer: Matteo Iervasi <matteoiervasi@gmail.com>
# Contributor: Joshua Chapman <john.chy99@gmail.com>
pkgname=logic-synthesis-bin
pkgver=1.4
pkgrel=1
epoch=
pkgdesc="Logic synthesis system from UC Berkeley."
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
source=("$pkgname-$pkgver.tar.gz")
        #"$pkgname-$pkgver.patch")
noextract=()
md5sums=('4561fdbf034ffa9674fd9b2391ed6c51')
validpgpkeys=()

# prepare() {
# 	cd "$pkgname-$pkgver"
# 	patch -p1 -i "$srcdir/$pkgname-$pkgver.patch"
# }

build() {
	cd "$srcdir"
	./configure --prefix=/usr
	make
}

check() {
	cd "$srcdir"
	make -k check
}

package() {
	cd "$srcdir"
	make DESTDIR="$pkgdir/" install
}

