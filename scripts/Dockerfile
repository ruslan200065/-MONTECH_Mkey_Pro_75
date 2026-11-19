FROM qmkfm/qmk_cli@sha256:b7d7fa8fb4432b569931de5ad59098cb788f440ed61a62c5126746b71aee0f4a
COPY requirements.txt /tmp/req.txt
RUN /opt/uv/tools/qmk/bin/python3 -m pip install --no-cache-dir -r /tmp/req.txt
