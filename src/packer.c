/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   packer.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jainavas <jainavas@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/23 19:45:19 by jainavas          #+#    #+#             */
/*   Updated: 2026/01/06 20:55:19 by jainavas         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/woody.h"
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

typedef struct
{
	uint64_t text_offset;
	uint64_t text_size;
	uint64_t text_addr;
	uint64_t old_entry;
	uint8_t key[KEY_SIZE];
} t_pack_info;

// Estructura que el stub espera (últimos 40 bytes del stub)
typedef struct __attribute__((packed))
{
	uint64_t text_addr; // 8 bytes
	uint64_t text_size; // 8 bytes
	uint64_t old_entry; // 8 bytes
	uint8_t key[16];	// 16 bytes
} stub_data_t;			// TOTAL: 40 bytes

static int encrypt_text(void *elf_data, t_pack_info *info)
{
	printf("[*] Encrypting .text section...\n");

	uint8_t *text_ptr = (uint8_t *)elf_data + info->text_offset;

	uint8_t S[256];
	rc4_init(S, info->key, KEY_SIZE);
	rc4_crypt(S, text_ptr, info->text_size);

	printf("[+] .text encrypted successfully\n");
	return 0;
}

static void *load_stub(size_t *stub_size)
{
	int fd = open("stub/stub.bin", O_RDONLY);
	if (fd < 0)
	{
		perror("open stub.bin");
		return NULL;
	}

	struct stat st;
	if (fstat(fd, &st) < 0)
	{
		perror("fstat stub");
		close(fd);
		return NULL;
	}

	*stub_size = st.st_size;
	void *stub = malloc(*stub_size);
	if (!stub)
	{
		perror("malloc stub");
		close(fd);
		return NULL;
	}

	if (read(fd, stub, *stub_size) != (ssize_t)*stub_size)
	{
		perror("read stub");
		free(stub);
		close(fd);
		return NULL;
	}

	close(fd);
	return stub;
}

static uint64_t align_to_page(uint64_t value)
{
	return (value + 0x1000 - 1) & ~(0x1000 - 1);
}

static int inject_stub(void **elf_data_ptr, size_t *elf_size_ptr,
					   t_pack_info *info)
{
	void *elf_data = *elf_data_ptr;
	size_t old_size = *elf_size_ptr;

	// Cargar stub
	size_t stub_code_size;
	void *stub_code = load_stub(&stub_code_size);
	if (!stub_code)
	{
		fprintf(stderr, "Error: Failed to load stub\n");
		return -1;
	}

	printf("[*] Loaded stub: %zu bytes\n", stub_code_size);

	// Verificar que el stub tiene espacio para los datos (últimos 40 bytes)
	if (stub_code_size < sizeof(stub_data_t))
	{
		fprintf(stderr, "Error: Stub too small\n");
		free(stub_code);
		return -1;
	}

	// Los datos están en los ÚLTIMOS 40 bytes del stub
	size_t data_offset = stub_code_size - sizeof(stub_data_t);

	printf("[*] Writing data at offset %zu in stub\n", data_offset);

	// Preparar datos para el stub
	stub_data_t stub_data = {
		.text_addr = info->text_addr,
		.text_size = info->text_size,
		.old_entry = info->old_entry,
	};
	memcpy(stub_data.key, info->key, KEY_SIZE);

	// Escribir los datos en el stub
	memcpy(stub_code + data_offset, &stub_data, sizeof(stub_data_t));

	printf("[*] Stub data: text_addr=0x%lx, text_size=%lu, old_entry=0x%lx\n",
		   stub_data.text_addr, stub_data.text_size, stub_data.old_entry);

	// Calcular nuevo tamaño alineado
	size_t aligned_stub_size = align_to_page(stub_code_size);
	size_t new_size = old_size + aligned_stub_size;

	// Realocar memoria
	void *new_elf = realloc(elf_data, new_size);
	if (!new_elf)
	{
		perror("realloc");
		free(stub_code);
		return -1;
	}

	// Limpiar el espacio nuevo
	memset(new_elf + old_size, 0, aligned_stub_size);

	Elf64_Ehdr *ehdr = (Elf64_Ehdr *)new_elf;
	Elf64_Phdr *phdr = (Elf64_Phdr *)(new_elf + ehdr->e_phoff);

	// Buscar el último segmento LOAD
	Elf64_Phdr *last_load = NULL;
	uint64_t max_vaddr = 0;

	for (int i = 0; i < ehdr->e_phnum; i++)
	{
		if (phdr[i].p_type == PT_LOAD)
		{
			uint64_t end_vaddr = phdr[i].p_vaddr + phdr[i].p_memsz;
			if (end_vaddr > max_vaddr)
			{
				max_vaddr = end_vaddr;
				last_load = &phdr[i];
			}
		}
	}

	if (!last_load)
	{
		fprintf(stderr, "Error: No LOAD segment found\n");
		free(stub_code);
		return -1;
	}

	// Calcular direcciones
	uint64_t stub_file_offset = old_size;

	// CRÍTICO: stub_vaddr debe calcularse basándose en el mapeo del segmento
	// La fórmula es: vaddr = segment_vaddr + (file_offset - segment_offset)
	uint64_t stub_vaddr = last_load->p_vaddr + (stub_file_offset - last_load->p_offset);

	printf("[*] Injecting stub at file offset 0x%lx\n", stub_file_offset);
	printf("[*] Stub virtual address: 0x%lx\n", stub_vaddr);

	// Copiar stub al final del archivo
	memcpy(new_elf + stub_file_offset, stub_code, stub_code_size);

	// DEBUG: Verificar que se copió bien
	printf("[DEBUG] First 16 bytes of stub_code: ");
	for (size_t i = 0; i < 16 && i < stub_code_size; i++)
		printf("%02x ", ((uint8_t *)stub_code)[i]);
	printf("\n");

	printf("[DEBUG] First 16 bytes at injection point: ");
	for (size_t i = 0; i < 16; i++)
		printf("%02x ", ((uint8_t *)new_elf)[stub_file_offset + i]);
	printf("\n");

	printf("[*] Extending last LOAD segment\n");

	// DEBUG
	printf("[DEBUG] Last LOAD before: offset=0x%lx vaddr=0x%lx filesz=0x%lx memsz=0x%lx\n",
		   last_load->p_offset, last_load->p_vaddr, last_load->p_filesz, last_load->p_memsz);
	printf("[DEBUG] stub_file_offset=0x%lx, stub_vaddr=0x%lx, aligned_stub_size=0x%zx\n",
		   stub_file_offset, stub_vaddr, aligned_stub_size);

	uint64_t old_memsz = last_load->p_memsz;
	uint64_t old_filesz = last_load->p_filesz;

	// Nuevo filesz: desde el inicio del segmento hasta el final del stub
	last_load->p_filesz = (stub_file_offset + aligned_stub_size) - last_load->p_offset;

	// Nuevo memsz: desde el inicio en vaddr hasta el final del stub en memoria
	last_load->p_memsz = (stub_vaddr + aligned_stub_size) - last_load->p_vaddr;

	last_load->p_flags |= PF_X | PF_R | PF_W;

	printf("[DEBUG] Last LOAD after: filesz=0x%lx memsz=0x%lx\n",
		   last_load->p_filesz, last_load->p_memsz);
	printf("[*] Segment extended: filesz 0x%lx->0x%lx, memsz 0x%lx->0x%lx\n",
		   old_filesz, last_load->p_filesz, old_memsz, last_load->p_memsz);

	// Cambiar entry point
	uint64_t old_entry = ehdr->e_entry;
	ehdr->e_entry = stub_vaddr;

	printf("[*] Entry point: 0x%lx -> 0x%lx\n", old_entry, stub_vaddr);

	// Actualizar punteros
	*elf_data_ptr = new_elf;
	*elf_size_ptr = new_size;

	free(stub_code);
	printf("[+] Stub injection completed\n");
	return 0;
}

static int write_woody(void *elf_data, size_t size, t_pack_info *info)
{
	(void)info;

	printf("[*] Writing woody (%zu bytes)...\n", size);

	int fd = open("woody", O_CREAT | O_WRONLY | O_TRUNC, 0755);
	if (fd < 0)
	{
		perror("open woody");
		return -1;
	}

	ssize_t written = write(fd, elf_data, size);
	if (written < 0 || (size_t)written != size)
	{
		perror("write");
		close(fd);
		return -1;
	}

	close(fd);
	printf("[+] woody created successfully\n");
	return 0;
}

int pack_elf(t_elf *elf, uint8_t *key)
{
	Elf64_Shdr *text = find_section(elf, ".text");
	if (!text)
	{
		fprintf(stderr, "Error: .text section not found\n");
		return -1;
	}

	printf("[*] Found .text: offset=0x%lx size=%lu bytes\n",
		   text->sh_offset, text->sh_size);

	t_pack_info info = {
		.text_offset = text->sh_offset,
		.text_size = text->sh_size,
		.text_addr = text->sh_addr,
		.old_entry = elf->ehdr->e_entry,
	};
	memcpy(info.key, key, KEY_SIZE);

	// Crear copia
	void *new_elf = malloc(elf->size);
	if (!new_elf)
	{
		perror("malloc");
		return -1;
	}
	memcpy(new_elf, elf->data, elf->size);
	size_t new_size = elf->size;

	// Cifrar .text
	if (encrypt_text(new_elf, &info) < 0)
	{
		free(new_elf);
		return -1;
	}

	// Inyectar stub
	if (inject_stub(&new_elf, &new_size, &info) < 0)
	{
		free(new_elf);
		return -1;
	}

	// Escribir resultado
	if (write_woody(new_elf, new_size, &info) < 0)
	{
		free(new_elf);
		return -1;
	}

	free(new_elf);
	printf("[+] Packing completed! Output: woody\n");
	return 0;
}