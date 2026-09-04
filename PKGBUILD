# Maintainer: beanie <jainarjav886@gmail.com>

pkgname=beanwm
pkgver=0.1.3
pkgrel=1
pkgdesc="A minimal dwindle-tiling X11 window manager written in C++23"
arch=('x86_64')
url="https://github.com/beaniemen/beanwm-v2"
license=('MIT')
depends=('libx11')
makedepends=('gcc')
backup=('etc/beanwm/config')
source=("$pkgname-$pkgver.tar.gz::$url/archive/v$pkgver.tar.gz")
sha256sums=('d81fc37440cde0121399739d472007a9b4df95181c9c1ba8f53e4f454ce57644')

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
