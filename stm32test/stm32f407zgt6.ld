MEMORY
{
    FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 1024K
    RAM   (rw) : ORIGIN = 0x20000000, LENGTH = 128K
    CCM   (rw) : ORIGIN = 0x10000000, LENGTH = 64K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    .isr_vector : { KEEP(*(.isr_vector)) } > FLASH
    .text       : { *(.text*) *(.rodata*) } > FLASH
    .ARM.exidx  : { __exidx_start = .; *(.ARM.exidx*); __exidx_end = .; } > FLASH

    _sidata = LOADADDR(.data);
    .data : {
        _sdata = .;
        *(.data*)
        _edata = .;
    } > RAM AT> FLASH

    .bss : {
        _sbss = .;
        *(.bss*)
        _ebss = .;
    } > RAM

    _end = .;
    end = .;

    /* CCM 紧耦合内存 (零等待, 适合 RTOS 任务栈 / 关键数据):
       用法: __attribute__((section(".ccm"))) 或 __attribute__((section(".ccmdata"))) */
    .ccm (NOLOAD) : {
        *(.ccm*)
    } > CCM
}
