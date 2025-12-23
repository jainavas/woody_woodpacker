/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   stub.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/23 19:45:12 by jainavas          #+#    #+#             */
/*   Updated: 2025/12/23 19:45:45 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>

// Esta estructura se llenará con los datos reales
typedef struct __attribute__((packed)) {
    uint64_t text_addr;
    uint64_t text_size;
    uint64_t old_entry;
    uint8_t  key[16];
} stub_data_t;

void rc4_init(uint8_t *S, uint8_t *key, size_t key_len)
{
    for (int i = 0; i < 256; i++)
        S[i] = i;
    
    int j = 0;
    for (int i = 0; i < 256; i++) {
        j = (j + S[i] + key[i % key_len]) % 256;
        uint8_t temp = S[i];
        S[i] = S[j];
        S[j] = temp;
    }
}

void rc4_crypt(uint8_t *S, uint8_t *data, size_t len)
{
    int i = 0, j = 0;
    
    for (size_t k = 0; k < len; k++) {
        i = (i + 1) % 256;
        j = (j + S[i]) % 256;
        
        uint8_t temp = S[i];
        S[i] = S[j];
        S[j] = temp;
        
        uint8_t rnd = S[(S[i] + S[j]) % 256];
        data[k] ^= rnd;
    }
}

// Este es el punto de entrada del stub
void stub_entry(stub_data_t *data)
{
    // 1. Hacer .text escribible (está cifrado, necesitamos descifrarlo in-place)
    mprotect((void *)data->text_addr, data->text_size, 
             PROT_READ | PROT_WRITE | PROT_EXEC);
    
    // 2. Descifrar
    uint8_t S[256];
    rc4_init(S, data->key, 16);
    rc4_crypt(S, (uint8_t *)data->text_addr, data->text_size);
    
    // 3. Restaurar permisos (solo lectura + ejecución)
    mprotect((void *)data->text_addr, data->text_size, 
             PROT_READ | PROT_EXEC);
    
    // 4. Imprimir mensaje
    const char *msg = "....WOODY....\n";
    write(1, msg, 14);
    
    // 5. Saltar al programa original
    void (*original_entry)(void) = (void (*)(void))data->old_entry;
    original_entry();
}
