# Maintainer: Matteo Iervasi <matteoiervasi@gmail.com>
# Contributor: Joshua Chapman <john.chy99@gmail.com>
pkgname=logic-synthesis-bin
pkgver=1.4
pkgrel=1
epoch=
pkgdesc="Logic synthesis system from UC Berkeley."
arch=('i686' 'x86_64')
url="https://jackhack96.github.io/logic-synthesis"
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
source=("https://github.com/joshuachp/logic-synthesis/releases/download/1.4/logic-synthesis-bin-1.4.tar.gz")
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

