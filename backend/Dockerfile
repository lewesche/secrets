FROM node

# sudo docker run --name secrets --rm -it -p 8000:8000 <imageID>

RUN mkdir /app
WORKDIR /app

COPY . .

RUN npm install

#Node deploys on port 8000
EXPOSE 8000

CMD ["npm", "run", "serve"]

