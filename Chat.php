<?php

namespace MyApp;

use Ratchet\MessageComponentInterface;
use Ratchet\ConnectionInterface;

class Chat implements MessageComponentInterface {
  protected $clients;
  
  public function __construct() {
    $this->clients = new \SplObjectStorage;
    echo "Congratulations! the server is now running\n";
  }
  
  public function onOpen(ConnectionInterface $conn) {
    // Store the new connection to send messages to later
    $this->clients->attach($conn);
    echo "New connection! ({$conn->resourceId})\n";
  }
  
  public function onMessage(ConnectionInterface $from, $msg) {
    //$msg = "id;tipo;estado"
    //separar los tres valores
    //$id=1254
    //$tipo="interruptor"
    //$estado=1
    
    list($id, $tipo, $estado) = explode (";",$msg);
      
    $dbhost = 'localhost';
    $dbuser = 'root';
    $dbpass = 'Feeney';
      
    $conn = mysql_connect($dbhost, $dbuser, $dbpass);
    mysql_select_db('BASEDATOS');
    
    //preguntar si existe el registro
    
    $sql = 'select id from dispositivos where id='.$id;
    $retval = mysql_query( $sql, $conn );
    row = mysql_fetch_array($retval, MYSQL_ASSOC);
    if ($row) {
      //existe
	//enviar "OK" a interruptor
      $sql = 'UPDATE dispositivos SET estado='.$estado.', socket='.$from->resourceId.' WHERE ID = '.$id;
    } else {
	//enviar "OK" a interruptor
      $sql = 'INSERT INTO dispositivos ( id, tipo, estado, socket, descripcion ) VALUES ('.$id.', '.$tipo.','.$estado.','.$from->resourceId.',"")';
    }
    $retval = mysql_query( $sql, $conn );
    
    //comunicar a los dispositivos asociados
    $sql ='select dispositivos.socket as socket from conexiones,dispositivos where conexiones.idorg='.$id.' and conexiones.iddst=dispositivos.id';
    $retval = mysql_query( $sql, $conn );
    $from->send("OK");
    while($row = mysql_fetch_array($retval, MYSQL_ASSOC)) {
      foreach ($this->clients as $client) {
        if ($row['socket'] == $client->resourceId) {     
	   $client->send($estado);
        }
      }
    } 
    
    $numRecv = count($this->clients) - 1;
    echo sprintf('Connection %d sending message "%s" to %d other connection%s' . "\n",
      $from->resourceId, $msg, $numRecv, $numRecv == 1 ? '' : 's');
    
    foreach ($this->clients as $client) {
      if ($from !== $client) {
        // The sender is not the receiver, send to each client connected
        $client->send($msg);
      }
    }
  }

  public function onClose(ConnectionInterface $conn) {
    // The connection is closed, remove it, as we can no longer send it messages
    $this->clients->detach($conn);
    echo "Connection {$conn->resourceId} has disconnected\n";
  }
  
  public function onError(ConnectionInterface $conn, \Exception $e) {
    echo "An error has occurred: {$e->getMessage()}\n";
    $conn->close();
  }
}
?>