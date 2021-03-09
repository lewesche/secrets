FROM node

RUN mkdir /app
WORKDIR /app

COPY . .

RUN npm install

#Node deploys on port 8000
EXPOSE 8000

CMD ["npm", "run", "serve"]

