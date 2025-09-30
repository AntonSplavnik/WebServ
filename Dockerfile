FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV NORM_VER 3.3.53

WORKDIR /home/root
VOLUME /home/root

RUN apt-get update -y && \
	apt-get install -y  lsb-release valgrind clang wget nano python3 \
						python3-pip build-essential cmake git curl python3-venv && \
						pip install norminette=="$NORM_VER" 
						#&> /dev/null && printf "\n\e[32m-----\ninstalled Norminette $NORM_VER\n\n\e[0m"

RUN wget https://raw.github.com/xicodomingues/francinette/master/bin/install.sh && chmod +x install.sh && \
	./install.sh && printf "\n\e[32m------\ninstalled Francinette\n\n\e[0m"

RUN	printf "\n\e[32m------\n READY TO GO! \n\n\e[0m" && \
	printf "\n\e[32m------\n docker run -it -v \$(pwd):/home/root ubuntu\n\n\e[0m" && \
	printf " \e[32mTo run a terminal inside the container , enjoy\n\n\e[0m"

CMD ["bash"]

# Build : 
#	docker build -t ubuntu .
# Run : 
#	docker run -it -v $(pwd):/home/root ubuntu