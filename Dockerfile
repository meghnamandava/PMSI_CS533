FROM ubuntu:14.04
MAINTAINER amkaushi@uwaterloo.ca

# Download and Install utilities
# ==============================
RUN rm /bin/sh && ln -s /bin/bash /bin/sh
RUN apt-get update \
  && apt-get install -y \
    vim \
		git 


RUN apt-get install -y swig gcc m4 python python-dev libgoogle-perftools-dev mercurial scons g++ build-essential zlib1g-dev wget

## cleanup
RUN apt-get clean && \
    cd /var/lib/apt/lists && rm -fr *Release* *Sources* *Packages* && \
    truncate -s 0 /var/log/*log
#Set env just in case
#ENV HOME /root
ENV PATH /usr/local/rvm/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin

# Download PMSI
RUN bash -l -c "wget --no-check-certificate https://git.uwaterloo.ca/caesr-pub/pmsi/repository/archive.tar.gz"
RUN bash -l -c "tar -zxvf archive.tar.gz; mv pmsi-master* pmsi; rm archive.tar.gz"

# Build PMSI
RUN bash -l -c "cd /pmsi/gem5-stable; scons ./build/X86_MSI_Snooping_One_Level_RT/gem5.opt -j2"
