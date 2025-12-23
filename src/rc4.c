/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   rc4.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/23 19:45:22 by jainavas          #+#    #+#             */
/*   Updated: 2025/12/23 19:45:23 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/woody.h"
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

// Inicializa el estado de RC4 con la clave
void rc4_init(uint8_t *S, uint8_t *key, size_t key_len)
{
    // S es un array de 256 bytes que representa el estado
    for (int i = 0; i < 256; i++)
        S[i] = i;
    
    int j = 0;
    for (int i = 0; i < 256; i++) {
        j = (j + S[i] + key[i % key_len]) % 256;
        
        // Swap S[i] y S[j]
        uint8_t temp = S[i];
        S[i] = S[j];
        S[j] = temp;
    }
}

// Cifra/descifra datos (es la misma operación)
void rc4_crypt(uint8_t *S, uint8_t *data, size_t len)
{
    int i = 0, j = 0;
    
    for (size_t k = 0; k < len; k++) {
        i = (i + 1) % 256;
        j = (j + S[i]) % 256;
        
        // Swap
        uint8_t temp = S[i];
        S[i] = S[j];
        S[j] = temp;
        
        // XOR con el byte generado
        uint8_t rnd = S[(S[i] + S[j]) % 256];
        data[k] ^= rnd;
    }
}

// Genera una clave aleatoria leyendo de /dev/urandom
void generate_key(uint8_t *key, size_t len)
{
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) {
        perror("open /dev/urandom");
        return;
    }
    
    read(fd, key, len);
    close(fd);
}