FROM ubuntu:latest
USER root
RUN apt-get update
RUN apt-get -y install curl findutils vim git jq gcc g++ sudo ntpdate python3 python3-pip wget

WORKDIR /
RUN git clone -b heroku https://github.com/sayefsakin/traveler-integrated
WORKDIR /traveler-integrated
RUN pip3 install -r requirements.txt
WORKDIR /traveler-integrated/profiling_tools/clibs
RUN python3 rp_extension_build.py
RUN mv _cCalcBin.*.so ..

WORKDIR /tmp
RUN mkdir traveler-integrated
WORKDIR /tmp/traveler-integrated
RUN wget https://github.com/sayefsakin/halide_notes/raw/master/BundledOTF2Data/77d9bd8d-c7c6-4060-a8ae-ad2efc4dd6fd.tar
RUN tar -xvf 77d9bd8d-c7c6-4060-a8ae-ad2efc4dd6fd.tar
RUN rm 77d9bd8d-c7c6-4060-a8ae-ad2efc4dd6fd.tar

WORKDIR /traveler-integrated
CMD ./serve.py -p 80