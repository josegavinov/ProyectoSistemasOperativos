# Proyecto de Sistemas Operativos  
## Arquitectura Distribuida con Broker, Gateway, Publishers y Subscribers  
**Materia:** Sistemas Operativos  
**Carrera:** Ingeniería en Computación  
**Lenguaje usado:** C  
**Estudiante:** Jose Gaviño Villacis
---

## 📌 Descripción General

Este proyecto implementa un sistema distribuido basado en el patrón **Publisher–Subscriber**, utilizando sockets TCP en **modo texto**, cumpliendo con los lineamientos de la asignatura de Sistemas Operativos.

El sistema está compuesto por los siguientes elementos:

- **Broker:** nodo central que gestiona suscripciones y reenvía mensajes.  
- **Gateway:** puente entre múltiples publishers y el broker.  
- **Publishers:** nodos simulados que envían métricas periódicas.  
- **Subscribers:** procesos que reciben datos de tópicos específicos.  

Los tópicos implementados incluyen:
- `temperature`
- `humidity`

Cada publisher envía dos métricas periódicas y los subscribers reciben únicamente las del tópico al que están suscritos.

---

## 🧩 Componentes del Sistema

### ✔️ Broker
- Administra tópicos y suscriptores.
- Reenvía mensajes a los subscribers correctos.
- Mantiene comunicación persistente con gateways y suscriptores.

### ✔️ Gateway
- Recibe conexiones de múltiples publishers.
- Reenvía mensajes crudos hacia el broker.
- Funciona como punto de acceso local para nodos.

### ✔️ Publishers
- Emulan nodos tipo IoT (Ej: ESP32).
- Envían datos periódicamente:
  - `temperature`
  - `humidity`
- Se conectan al gateway.

### ✔️ Subscribers
- Se conectan al broker.
- Se suscriben a tópicos usando:
- Reciben datos en tiempo real.



