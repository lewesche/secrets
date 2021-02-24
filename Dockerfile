FROM rust

RUN mkdir /app
WORKDIR /app
RUN mkdir /var/secrets_data

COPY . .

# Compile and install the c++ binary
RUN chmod +x *.sh
RUN ./install.sh

# Rocket.rs framework requires nightly
RUN rustup default nightly
RUN rustup override set nightly

RUN cargo build --manifest-path=backend/Cargo.toml

#Rocket deploys on port 8000
EXPOSE 8000

CMD ["cargo", "run", "--manifest-path=backend/Cargo.toml", "--release"]

