import { Server } from "mock-socket";
import {Batteries} from "./mockData";

let mockServer: Server | null = null;

export function setupMockWebSocket() {
  
  if (mockServer) {
    return;
  }

  mockServer = new Server("ws://localhost:8888/browser_ws");

  mockServer.on("connection", (socket) => {
    socket.on("message", (data) => {
      socket.send(data);
    });

    setInterval(() => {
      socket.send(JSON.stringify(Batteries));
    }, 1000);
  });
}