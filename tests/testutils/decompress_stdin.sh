#!/bin/sh
type uncompress > /dev/null && exec uncompress
type gzip > /dev/null && exec gzip -df
exit 1
