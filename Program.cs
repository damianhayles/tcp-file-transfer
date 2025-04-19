using System;
using System.Net.Sockets;
using System.Text;
using System.IO;

class TCPClient
{
    const int BUF = 1024;

    static void Main()
    {
        string server = "127.0.0.1";
        int port = 59589;

        try {
            using TcpClient client = new TcpClient(server, port);
            using NetworkStream stream = client.GetStream();

            // Read greeting from the server
            byte[] buffer = new byte[BUF];
            int bytes = stream.Read(buffer, 0, buffer.Length);
            Console.WriteLine("--C# Client--" + Encoding.ASCII.GetString(buffer, 0, bytes));

            while (true)
            {
                Console.Write("Enter filepath to send (or type DONE to finish): ");
                string? filepath = Console.ReadLine();

                if (filepath == null || filepath == "DONE")
                {
                    byte[] doneBytes = Encoding.UTF8.GetBytes(filepath ?? string.Empty);
                    stream.Write(doneBytes, 0, doneBytes.Length);
                    break;
                }

                if (!File.Exists(filepath))
                {
                    Console.WriteLine("File not found.");
                    return;
                }

                // Send filepath
                byte[] pathBytes = Encoding.UTF8.GetBytes(filepath);
                stream.Write(pathBytes, 0, pathBytes.Length);

                // Wait for server confirmation
                bytes = stream.Read(buffer, 0, buffer.Length);
                Console.WriteLine("Server: " + Encoding.ASCII.GetString(buffer, 0, bytes));

                // Send file data in packets
                using FileStream file = new FileStream(filepath, FileMode.Open, FileAccess.Read);
                byte[] fileBuf = new byte[BUF];

                long totalSent = 0;
                long totalSize = file.Length;

                int bytesRead;
                while ((bytesRead = file.Read(fileBuf, 0, BUF)) > 0)
                {
                    // Build a packet with header followed by payload
                    byte[] packet = new byte[4 + bytesRead];
                    Array.Copy(BitConverter.GetBytes(bytesRead), 0, packet, 0, 4);
                    Array.Copy(fileBuf, 0, packet, 4, bytesRead);

                    stream.Write(packet, 0, packet.Length);

                    totalSent += bytesRead;
                    double progress = (double)totalSent / totalSize;
                    int barWidth = 50;
                    int pos = (int)(barWidth * progress);
                    Console.Write($"\r[");
                    for (int i = 0; i < barWidth; i++)
                    {
                        if (i < pos) Console.Write("=");
                        else if (i == pos) Console.Write(">");
                        else Console.Write(" ");
                    }
                    Console.Write($"] {progress * 100,5:0.0}%");
                }
                Console.WriteLine();

                // Send EOF packet (0 bytes)
                byte[] endPacket = BitConverter.GetBytes(0);
                stream.Write(endPacket, 0, endPacket.Length);
                stream.Flush();

                Console.WriteLine("Transfer Complete!");
            }
            
        }
        catch (Exception ex)
        {
            Console.WriteLine("Error: " + ex.Message);
        }
    }
}
