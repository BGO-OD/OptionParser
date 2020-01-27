FROM ubuntu:latest

MAINTAINER nobody@a.de
LABEL description="Doxygen documentation generation"

RUN apt update && apt install -y \
    build-essential bash cmake \
    doxygen graphviz && \
    rm -rf /var/lib/apt/lists/*

ADD run_action.sh /run_action.sh
ENTRYPOINT ["/bin/bash", "-x"]
CMD ["/run_action.sh"]
