import struct
import random

# Generate a synthetic ITCH-like file of Add Order messages (type 'A')
# Each message is 36 bytes: msg_type(1), stock_locate(2), tracking(2),
# timestamp(6), order_ref(8), side(1), shares(4), stock(8), price(4)
def generate_itch_file(filename, num_messages):
    with open(filename, 'wb') as f:
        for _ in range(num_messages):
            msg_type = b'A'                      # Add Order
            stock_locate = b'\x00\x00'           # 2 bytes
            tracking = b'\x00\x00'               # 2 bytes
            timestamp = b'\x00' * 6              # 6 bytes
            order_ref = b'\x00' * 8              # 8 bytes
            side = random.choice([b'B', b'S'])   # Buy or Sell
            shares = struct.pack('>I', random.randint(100, 1000))  # big-endian
            stock = b'ABCDEFGH'                   # 8 chars
            price = struct.pack('>I', random.randint(10000, 20000)) # 100.00 - 200.00
            f.write(msg_type + stock_locate + tracking + timestamp +
                    order_ref + side + shares + stock + price)

if __name__ == "__main__":
    generate_itch_file("sample_synthetic.ITCH", 100000)  # 100k packets
    print("Generated sample_synthetic.ITCH")