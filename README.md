# BYOR

## Building
```
docker build -t byor .
```

## Running
### Run the server
```
docker run -it --rm --name byor byor
./server
```
### Run the client
```
docker exec -it byor bash
./client
```

