# Maintainer: beanie <your@email.com>

pkgname=beanwm
pkgver=0.1.0
pkgrel=1
pkgdesc="A minimal dwindle-tiling X11 window manager written in C++23"
arch=('x86_64')
url="https://github.com/beaniemen/beanwm-v2"
license=('MIT')
depends=('libx11')
makedepends=('gcc')
backup=('etc/beanwm/config')
source=("$pkgname-$pkgver.tar.gz::$url/archive/v$pkgver.tar.gz")
sha256sums=('SKIP')

build() {
    cd "$pkgname-$pkgver"
    make -j"$(nproc)"
}

package() {
    cd "$pkgname-$pkgver"
    make install DESTDIR="$pkgdir"
    install -Dm644 LICENSE "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
    install -Dm644 README.md "$pkgdir/usr/share/doc/$pkgname/README.md"
}
