FROM python:latest

COPY ./script /script

RUN pip3 install pyopenssl

CMD [ "python3", "/script/generator.py" ]