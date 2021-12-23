#!/bin/bash

read -r -d '' content
content=${content//'\'/'\\'}
content=${content//$'\t'/'\t'}
content=${content//$'\n'/$' \\n\\\n'}
content=${content//$'\r'/'\r'}
content=${content//'"'/'\"'}
echo -n "\"$content\""
