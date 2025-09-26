# Usa la imagen oficial de Node.js (versión estable, por ejemplo 18)
FROM node:18

# Crear un directorio de trabajo
WORKDIR /usr/src/app

# Copiar los archivos del proyecto al contenedor
COPY package*.json ./
RUN npm install

# Copiar el resto del código fuente al contenedor
COPY . .

# Exponer el puerto 3000 (servidor web)
EXPOSE 3000

# Comando para ejecutar el servidor Node.js
CMD ["npm", "run", "start"]
