#!/bin/bash

if [ ! -d "${HOME}/.ibrio/" ]; then
    mkdir -p ${HOME}/.ibrio/
fi

if [ ! -f "${HOME}/.ibrio/ibrio.conf" ]; then
    cp /ibrio.conf ${HOME}/.ibrio/ibrio.conf
fi

exec "$@"
