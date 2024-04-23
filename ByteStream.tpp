template <typename T> const T ByteStream::readBytes(int num) {
	if (num > (int)sizeof(T))
		throw std::runtime_error("Read bytes a smaller type.");
	T value = readByte();
	for (int i = 0; i < num - 1; i++)
		value = (value << 8) | readByte();
	return value;
}
