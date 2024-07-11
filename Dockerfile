FROM condaforge/miniforge3

SHELL ["/bin/bash", "-c"]

# Clone SpiceQL repo instead of copying?
#git clone git@github.com:DOI-USGS/SpiceQL.git /repo

RUN mkdir /repo    
COPY . /repo
RUN echo $(ls /repo)
RUN chmod -R 755 /repo

# Set repo root env
ENV SPICEQL_REPO_ROOT /repo

# Need to mount ISIS data area
ENV SSPICE_DEBUG=TRUE
ENV SPICEROOT=/mnt/isis_data
ENV SPICEQL_LOG_LEVEL=TRACE

# CMD ["/bin/bash"]
# RUN /bin/bash -c "source activate spiceql"
RUN mamba env create -f ${SPICEQL_REPO_ROOT}/environment.yml -n spiceql && \
    source /opt/conda/etc/profile.d/conda.sh && \
    conda init && \
    conda activate spiceql && \
    conda install -c conda-forge spiceql && \
    echo "source activate spiceql" > ~/.bashrc && \
    cd ${SPICEQL_REPO_ROOT}/fastapi

ENV PATH /opt/conda/envs/spiceql/bin:$PATH

WORKDIR ${SPICEQL_REPO_ROOT}/fastapi

EXPOSE 8080

ENTRYPOINT [ "uvicorn", "app.main:app", "--reload", "--port" , "8080" ]
