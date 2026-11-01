FROM fedora:43
RUN dnf -y --setopt=install_weak_deps=False install \
      git cmake ninja-build clang gcc-c++ make patch sed gzip bzip2 xz curl tar \
      python3-pip llvm zip unzip libxml2-devel openssl-devel zlib-devel && dnf clean all
RUN git clone --depth 1 https://github.com/tpoechtrager/osxcross /opt/osxcross
RUN curl -fsSL -o /opt/osxcross/tarballs/MacOSX14.4.sdk.tar.xz \
      https://github.com/joseluisq/macosx-sdks/releases/download/14.4/MacOSX14.4.sdk.tar.xz
RUN cd /opt/osxcross && UNATTENDED=1 ENABLE_ARCHS="arm64 x86_64" ./build.sh
ENV PATH=/opt/osxcross/target/bin:$PATH
WORKDIR /work
