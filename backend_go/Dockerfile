FROM golang

# sudo docker run --name secrets --rm -d -p 8000:8000 <imageID>

RUN mkdir /app
WORKDIR /app

COPY . .

RUN go get ./...

#deploys on port 8000
EXPOSE 8000

CMD ["go", "run", "."]

