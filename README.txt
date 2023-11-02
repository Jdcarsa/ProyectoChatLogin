
Elaborado por: Jesus David Cardenas Sandoval <jcardenass@unicauca.edu.co>

Proyecto chat-login.

-server.c: Programa servidor.
El proceso servidor deberá recibir una conexión del cliente, verificar que el
nombre de usuario especificado no se encuentre usado en el momento y crear
una nueva tarea (hilo) que atenderá esta conexión.
El hilo se encargará de recibir mensajes del cliente conectado y reenviarlo
a los demás clientes que también se encuentren conectados para los mensajes
públicos, o sólo al usuario seleccionado si se envía un mensaje privado.

-cliente.c: Programa cliente.
El cliente deberá solicitar por entrada estándar (o como argumento de línea
de comandos) el login (nombre de usuario) con el cual se registrará en el chat.

Cuando un cliente se conecte al servidor de manera exitosa, el cliente de-
berá enviar un mensaje al servidor, informando el nombre de usuario deseado.

El servidor deberá responder con un mensaje de confirmación, o de error si el
nombre de usuario está siendo usado.
