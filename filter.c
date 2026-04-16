#include "filter.h"


#include "filter.h"

#include "filter.h"

int32_t ApplyMedianFilter18(MedianFilter_t *f, int32_t new_val)
{
    uint8_t i, j;
    int32_t key;
    
    // 1. HENÜZ TAM DOLMAMISSA
    if (f->count < FILTER_SIZE) {
        // Yeni degeri ekle
        f->samples[f->count] = new_val;
        f->count++;
        
        // Ilk degerler için geçici çözüm: 
        // Filtre dolana kadar mevcut degerlerin medyanini döndür
        if (f->count >= 3) {
            // Mevcut degerleri sort_buffer'a kopyala
            for (i = 0; i < f->count; i++) {
                f->sort_buffer[i] = f->samples[i];
            }
            
            // Sirala (sadece dolu kismi)
            for (i = 1; i < f->count; i++) {
                key = f->sort_buffer[i];
                j = i;
                while ((j > 0) && (f->sort_buffer[j-1] > key)) {
                    f->sort_buffer[j] = f->sort_buffer[j-1];
                    j--;
                }
                f->sort_buffer[j] = key;
            }
            
            // Medyani döndür
            return f->sort_buffer[f->count / 2];
        }
        
        return new_val;  // Yeterli veri yoksa dogrudan döndür
    }
    
    // 2. TAM DOLMUS - Normal islem
    // Yeni degeri dairesel buffer'a yaz
    f->samples[f->index] = new_val;
    f->index++;
    if (f->index >= FILTER_SIZE) {
        f->index = 0;
    }
    
    // 3. Siralama için kopyala
    for (i = 0; i < FILTER_SIZE; i++) {
        f->sort_buffer[i] = f->samples[i];
    }
    
    // 4. Insertion Sort (tüm buffer için)
    for (i = 1; i < FILTER_SIZE; i++) {
        key = f->sort_buffer[i];
        j = i;
        while ((j > 0) && (f->sort_buffer[j-1] > key)) {
            f->sort_buffer[j] = f->sort_buffer[j-1];
            j--;
        }
        f->sort_buffer[j] = key;
    }
    
    // 5. Medyani döndür
    return f->sort_buffer[FILTER_SIZE / 2];
}