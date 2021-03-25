FROM rust

# sudo docker run --name secrets_frontend --rm -d -p 8080:8080 <imageID>

RUN mkdir /app
WORKDIR /app

COPY . .

# Install Nodejs/npm
RUN apt-get update
RUN apt-get install curl
RUN curl -fsSL https://deb.nodesource.com/setup_15.x | bash
RUN apt-get install -y nodejs
RUN node -v
RUN npm -v

RUN apt install pkg-config
RUN cargo install wasm-pack

RUN npm install
RUN npm run build

#Webpack deploys on port 8080
EXPOSE 8080

CMD ["npm", "run", "serve"]

