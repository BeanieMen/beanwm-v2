# Maintainer: beanie <jainarjav886@gmail.com>

pkgname=beanwm
pkgver=0.1.1
pkgrel=1
pkgdesc="A minimal dwindle-tiling X11 window manager written in C++23"
arch=('x86_64')
url="https://github.com/beaniemen/beanwm-v2"
license=('MIT')
depends=('libx11')
makedepends=('gcc')
backup=('etc/beanwm/config')
source=("$pkgname-$pkgver.tar.gz::$url/archive/v$pkgver.tar.gz")
sha256sums=('5478867239bd020df7ee8826788f968c24d84c6ed566e093c0973b695568d2b8')

build() {
    if [ -d "$srcdir/$pkgname-$pkgver" ]; then
        cd "$srcdir/$pkgname-$pkgver"
    elif [ -d "$srcdir/beanwm-v2-$pkgver" ]; then
        cd "$srcdir/beanwm-v2-$pkgver"
    elif [ -f "$startdir/Makefile" ]; then
        cd "$startdir"
    fi
    make -j"$(nproc)"
}

package() {
    if [ -d "$srcdir/$pkgname-$pkgver" ]; then
        cd "$srcdir/$pkgname-$pkgver"
    elif [ -d "$srcdir/beanwm-v2-$pkgver" ]; then
        cd "$srcdir/beanwm-v2-$pkgver"
    elif [ -f "$startdir/Makefile" ]; then
        cd "$startdir"
    fi
    make install DESTDIR="$pkgdir"
    [ -f LICENSE ]   && install -Dm644 LICENSE   "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
    [ -f README.md ] && install -Dm644 README.md "$pkgdir/usr/share/doc/$pkgname/README.md"
}
