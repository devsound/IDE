require 'log'
require 'serialport'
require 'swd-bitbang'

class MchckBitbangSwd < BitbangSwd
  CMD_HANDSHAKE = "?SWD?"
  CMD_HANDSHAKE_REPLY = "!SWD1"
  CMD_WRITE_WORD = 0x90
  CMD_WRITE_BITS = 0xa0
  CMD_READ_WORD = 0x10
  CMD_READ_BITS = 0x20
  CMD_CYCLE_CLOCK = 0x28
  CMD_RESET_LOW = 0xb0
  CMD_RESET_HIGH = 0xb1

  def initialize(opt = {})
    super
    @outbuf = []

    Log(:phys, 1){ 'connecting to device %s' % opt[:dev] }
    @dev = SerialPort.new(opt[:dev], opt[:speed] || 9600)
    @dev.flow_control = SerialPort::NONE

    # The Arduino automatically resets when connecting on the serial
    # port.  Once the Ardunio resets, it will run its bootloader,
    # which will just discard data arriving on the serial port, until
    # it timeouts and passes control to the sketch.  Therefore, we
    # have to poll the port a bit, until we get a reply, or it took
    # unreasonably long.
    @dev.read_timeout = 100     # 100 ms timeout for handshaking
    tries = 20                  # try for 2 seconds
    tries.times do
      Log(:phys, 3){ 'sending handshake' }
 	  $stderr.puts "sending handshake"
      @dev.write(CMD_HANDSHAKE)
      reply = @dev.read(CMD_HANDSHAKE_REPLY.length)
      Log(:phys, 3){ 'received handshake %s' % reply.inspect }

      tries -= 1
      if reply != CMD_HANDSHAKE_REPLY
          t1 = Time.now
          while (Time.now - t1) < 0.5 do
            @dev.flush_input()
          end
          next
      end
      next if reply.nil? && tries >= 0

      if reply != CMD_HANDSHAKE_REPLY
        raise RuntimeError, "not a MC HCK SWD bitbang adapter: received `#{reply}'"
      end
      break
    end
    @dev.read_timeout = 0

    @cur_dir = :out
  end

  def flush!
    return if @outbuf.empty?
    data = @outbuf.pack('C*')
    Log(:phys, 2){ ['flush', hexify(data)] }
    @dev.write(data)
    @outbuf = []
  end

  def turn(dir)
    if @cur_dir != dir
      Log(:phys, 2){ 'turning %s' % dir }
      @outbuf << CMD_CYCLE_CLOCK
      @cur_dir = dir
    end
  end

  def write_bytes(bytes)
    bytes = bytes.bytes.to_a

    while bytes.count >= 4
      this_word = bytes.slice!(0..3)
      @outbuf << CMD_WRITE_WORD
      @outbuf += this_word
    end

    bytes.each do |b|
      @outbuf << (CMD_WRITE_BITS | (8 - 1))
      @outbuf << b
    end
  end

  def write_bits(byte, len)
    return if len == 0

    @outbuf << (CMD_WRITE_BITS | (len - 1))
    @outbuf << byte.ord
  end

  def write_word_and_parity(word, parity)
    turn(:out)
    write_bytes([word].pack('V'))
    write_bits(parity, 1)
  end

  def write_cmd(cmd)
    turn(:out)
    write_bytes(cmd)
    turn(:in)

    # read ack
    @outbuf << (CMD_READ_BITS | (3 - 1))

    ack, = expect_bytes(1).unpack('C')
    Log(:phys, 2){ 'read ack: %d' % ack }
    ack
  end

  def reset_low()
    Log(:kinetis, 1){ "reset low" }
    @outbuf << CMD_RESET_LOW
  end
  
  def reset_high()
    Log(:kinetis, 1){ "reset high" }
    @outbuf << CMD_RESET_HIGH
  end

  def read_word_and_parity
    raise RuntimeError, "invalid line direction" if @cur_dir != :in

    turn(:in)
    @outbuf << CMD_READ_WORD
    @outbuf << CMD_READ_BITS
    turn(:out)

    data = expect_bytes(5)

    ret, par = data.unpack('VC')
    Log(:phys, 2){ 'read word: %08x, parity %d' % [ret, par] }
    [ret, par]
  end

  def expect_bytes(num)
    flush!
    Log(:phys, 5){ 'expecting %d data bytes' % num }
    data = @dev.read(num)
    Log(:phys, 3){ ['read:', hexify(data)] }
    data
  end
end

if $0 == __FILE__
  s = MchckBitbangSwd.new(:dev=> ARGV[0])

  s.raw_out(255.chr * 7)
  s.raw_out([0xe79e].pack('v'))
  s.raw_out(255.chr * 7)
  s.raw_out(0.chr)
  s.flush!
  begin
    puts "IDCODE: %08x" % s.transact(0xa5)
  rescue
    s.raw_out(255.chr * 7)
    s.raw_out(0.chr)
    puts "IDCODE(2): %08x" % s.transact(0xa5)
  end
end
