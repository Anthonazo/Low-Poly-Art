# Image & Video Filters: Low Poly Effect

Este proyecto implementa un conjunto de filtros de imágenes y videos en tiempo real, destacando el efecto **Low Poly** en C++ utilizando técnicas avanzadas de procesamiento de imágenes como el **Canny Filter**, generación de puntos clave, triangulación de Delaunay y renderizado final sobre una imagen de San Basilio.

## Estructura del Proyecto

El código fuente está organizado en las siguientes carpetas:  

- **`src/Image Filter`**  
  Contiene la implementación de los filtros aplicados a imágenes estáticas. Aquí se calcula el efecto **Low Poly** paso a paso:
  1. Aplicación del filtro **Canny** para detectar bordes.
  2. Extracción de puntos clave a partir de los bordes detectados.
  3. Realización de la **triangulación de Delaunay** con los puntos clave.
  4. Renderizado final del efecto **Low Poly** sobre la imagen de San Basilio.

- **`src/Video Filter`**  
  Proporciona la funcionalidad para aplicar el filtro **Low Poly** en tiempo real sobre un video. Este módulo muestra dos ventanas simultáneamente:
  1. Una ventana con el video original en tiempo real.
  2. Otra ventana con el video procesado que incluye el efecto **Low Poly**.
  
  Además, los **FPS** (Frames por Segundo) del procesamiento se calculan utilizando las funcionalidades desarrolladas en `Image Filter`.

## Efecto Low Poly

El efecto **Low Poly** transforma imágenes en representaciones simplificadas mediante polígonos. El flujo de trabajo incluye los siguientes pasos:  
1. **Canny Filter:** Se utiliza para identificar los bordes de la imagen.  
2. **Puntos Clave:** Los bordes se procesan para seleccionar puntos clave significativos.  
3. **Triangulación de Delaunay:** Genera una malla de triángulos con los puntos clave seleccionados.  
4. **Renderizado Final:** Los triángulos se rellenan y se aplican sobre la imagen original, obteniendo un efecto artístico y simplificado.  

## Ejecución en Tiempo Real

En la carpeta `Video Filter`, se ejecuta el filtro en tiempo real mostrando:
- Una ventana con el **video original**.
- Una ventana con el **video procesado** que incluye el filtro **Low Poly**.
- Información sobre los **FPS** del procesamiento en tiempo real.

## Requisitos del Sistema

- **Lenguaje de programación:** C++  
- **Bibliotecas necesarias:** 
  - OpenCV (para procesamiento de imágenes y video)  

## Ejecución del Proyecto

1. Clona el repositorio:  
   ```bash
   git clone <URL_DEL_REPOSITORIO>
   cd <CARPETA_DEL_PROYECTO>
