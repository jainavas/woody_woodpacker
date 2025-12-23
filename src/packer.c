/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   packer.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/23 19:45:19 by jainavas          #+#    #+#             */
/*   Updated: 2025/12/23 19:45:20 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/woody.h"
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

// Vamos a guardar info importante sobre lo que ciframos
typedef struct {
    uint64_t text_offset;   // Dónde está .text en el archivo
    uint64_t text_size;     // Cuántos bytes cifrar
    uint64_t text_addr;     // Dirección virtual de .text en memoria
    uint64_t old_entry;     // Entry point original
    uint8_t  key[KEY_SIZE]; // La clave para descifrar
} t_pack_info;

static int encrypt_text(void *elf_data, t_pack_info *info)
{
    printf("[*] Encrypting .text section...\n");
    
    // Puntero al inicio de .text dentro del ELF
    uint8_t *text_ptr = (uint8_t *)elf_data + info->text_offset;
    
    // Inicializar RC4
    uint8_t S[256];
    rc4_init(S, info->key, KEY_SIZE);
    
    // Cifrar
    rc4_crypt(S, text_ptr, info->text_size);
    
    printf("[+] .text encrypted successfully\n");
    return 0;
}

int pack_elf(t_elf *elf, uint8_t *key)
{
    // 1. Buscar la sección .text
    Elf64_Shdr *text = find_section(elf, ".text");
    if (!text) {
        fprintf(stderr, "Error: .text section not found\n");
        return -1;
    }
    
    printf("[*] Found .text: offset=0x%lx size=%lu bytes\n", 
           text->sh_offset, text->sh_size);
    
    // 2. Guardar información importante
    t_pack_info info = {
        .text_offset = text->sh_offset,
        .text_size = text->sh_size,
        .text_addr = text->sh_addr,      // Dirección virtual en memoria
        .old_entry = elf->ehdr->e_entry,
    };
    memcpy(info.key, key, KEY_SIZE);
    
    // 3. Crear copia modificable del ELF
    void *new_elf = malloc(elf->size);
    if (!new_elf) {
        perror("malloc");
        return -1;
    }
    memcpy(new_elf, elf->data, elf->size);
    
    // 4. Cifrar .text en la copia
    if (encrypt_text(new_elf, &info) < 0) {
        free(new_elf);
        return -1;
    }
    
    // 5. Inyectar stub y modificar entry point
    if (inject_stub(new_elf, elf->size, &info) < 0) {
        free(new_elf);
        return -1;
    }
    
    // 6. Guardar el resultado
    if (write_woody(new_elf, elf->size, &info) < 0) {
        free(new_elf);
        return -1;
    }
    
    free(new_elf);
    printf("[+] Packing completed! Output: woody\n");
    return 0;
}
