FROM condaforge/miniforge3

SHELL ["/bin/bash", "-c"]

# Clone SpiceQL repo instead of copying?
#git clone git@github.com:DOI-USGS/SpiceQL.git /repo

# RUN git clone https://github.com/chkim-usgs/SpiceQL.git /repo --recursive --branch docker
# RUN echo $(ls /repo)
# RUN chmod -R 755 /repo

RUN apt-get update && apt-get install build-essential -y

ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update
RUN apt-get install nginx curl -y 
COPY fastapi/config/nginx.conf /etc/nginx/nginx.con

# CMD ["/bin/bash"]
# RUN /bin/bash -c "source activate spiceql"

 RUN mkdir /repo    
 COPY . /repo
 RUN echo $(ls /repo)
 RUN chmod -R 755 /repo

# Set repo root env
ENV SPICEQL_REPO_ROOT /repo

RUN mamba env create -f ${SPICEQL_REPO_ROOT}/environment.yml -n spiceql && \
    source /opt/conda/etc/profile.d/conda.sh && \
    conda init && \
    conda activate spiceql && \
    # conda install -c conda-forge spiceql && \
    echo "source activate spiceql" > ~/.bashrc && \ 
    conda activate spiceql && \ 
    cd $SPICEQL_REPO_ROOT && mkdir -p build && cd build && \ 
    cmake .. -DCMAKE_BUILD_TYPE=Release DCMAKE_INSTALL_PREFIX=$CONDA_PREFIX -DSPICEQL_BUILD_TESTS=OFF -DSPICEQL_BUILD_DOCS=OFF -GNinja && \ 
    ninja install 

RUN cd ${SPICEQL_REPO_ROOT}/fastapi
ENV PATH /opt/conda/envs/spiceql/bin:$PATH


WORKDIR ${SPICEQL_REPO_ROOT}/fastapi

EXPOSE 8080

copy Entrypoint.sh /
RUN chmod +x /Entrypoint.sh

RUN mkdir /mnt/isisdata/

ENTRYPOINT ["/bin/bash", "/Entrypoint.sh"]
