#!/bin/bash

rm -r dist
npm run build
sudo cp -r dist /var/www/html/
sudo cp secrets.html /var/www/html/

npm run serve
