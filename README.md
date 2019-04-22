# harb

A **H**eap **A**nalyzer for **R**uby. Early yet. Only built on Linux.

#### Building
`make`

#### Usage
`harb <heap_dump_file>`

#### Example

```
[vagrant] ~/src/harb (master) $ ./harb staging.json
parsing staging.json .......................... done: 2489364 heap objects
harb> help
You can run the following commands:

             print - Prints heap info for the address specified
          rootpath - Display the root path for the object specified
              help - Displays this message
              quit - Exits the program

harb> print 0x7f29fc044598
    0x7f29fc044598: "STRING"
             value: "Yellow/Navy"
              size: 40
   referenced from: [
                      0x7f2a28829318 (HASH: size 21)
                      0x7f2a1d4b5fe8 (HASH: size 21)
                    ]
harb> rootpath 0x7f2a0e642d70

root path to 0x7f2a0e642d70:
                      ROOT (global_tbl)
                      0x7f2a0e442598 (DATA: VM)
                      0x7f2a0e42a3a8 (ARRAY: size 564)
                      0x7f2a0fa1cda0 (ARRAY: size 134)
                      0x7f2a1d4a2380 (DATA: proc)
                      0x7f2a1d4a23d0 (DATA: VM/env)
                      0x7f2a1d4a2448 (ARRAY: size 66)
                      0x7f2a0ee336d8 (ARRAY: size 82)
                      0x7f2a0ec8c5a0 (OBJECT: ProductVariant)
                      0x7f2a0ec8c320 (HASH: size 1)
                      0x7f2a0ecb7ea8 (OBJECT: ActiveRecord::Associations::BelongsToAssociation)
                      0x7f2a0e9b55c0 (OBJECT: Product)
                      0x7f2a0ee18540 (OBJECT: ActiveRecord::Associations::CollectionProxy::ActiveR...)
                      0x7f2a0e642d98 (OBJECT: ActiveRecord::Associations::HasManyAssociation)
                      0x7f2a0e642d70 (ARRAY: size 66)

harb>
```

#### Dependencies
- libgoogle-perftools-dev
- libcityhash-dev
- libreadline-dev
- sparsehash
