# Maintainer: beanie <jainarjav886@gmail.com>

pkgname=beanwm
pkgver=0.1.6
pkgrel=1
pkgdesc="A minimal dwindle-tiling X11 window manager written in C++23"
arch=('x86_64')
url="https://github.com/beaniemen/beanwm-v2"
license=('MIT')
depends=('libx11')
makedepends=('gcc')
backup=('etc/beanwm/config')
source=("$pkgname-$pkgver.tar.gz::$url/archive/v$pkgver.tar.gz")
sha256sums=('3c6761eca054d88cc04da0abebb02aba3f5a593f7d6e1571f981d982f964257d')

build() {
    cd "$srcdir/beanwm-v2-$pkgver"
    make -j"$(nproc)"
}

package() {
    cd "$srcdir/beanwm-v2-$pkgver"

    make install DESTDIR="$pkgdir"

    install -Dm644 LICENSE \
        "$pkgdir/usr/share/licenses/$pkgname/LICENSE"

    if [ -f README.md ]; then
        install -Dm644 README.md \
            "$pkgdir/usr/share/doc/$pkgname/README.md"
    fi
}